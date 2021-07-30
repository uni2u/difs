/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019, Regents of the University of California.
 *
 * This file is part of NDN repo-ng (Next generation of NDN repository).
 * See AUTHORS.md for complete list of repo-ng authors and contributors.
 *
 * repo-ng is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * repo-ng is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * repo-ng, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "write-handle.hpp"

#include <ndn-cxx/util/logger.hpp>
#include <ndn-cxx/util/random.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>

#include "manifest/manifest.hpp"
#include "util.hpp"
#include "repo.hpp"

namespace repo {

NDN_LOG_INIT(repo.WriteHandle);

static const int DEFAULT_CREDIT = 12;
static const bool DEFAULT_CANBE_PREFIX = false;
static const milliseconds MAX_TIMEOUT(60_s);
static const milliseconds NOEND_TIMEOUT(10000_ms);
static const milliseconds PROCESS_DELETE_TIME(10000_ms);
static const milliseconds DEFAULT_INTEREST_LIFETIME(4000_ms);

WriteHandle::WriteHandle(Face& face, KeySpaceHandle& keySpaceHandle, RepoStorage& storageHandle, ndn::mgmt::Dispatcher& dispatcher,
                         Scheduler& scheduler, Validator& validator,
                         ndn::Name const& clusterNodePrefix, std::string clusterPrefix)
  : CommandBaseHandle(face, storageHandle, scheduler, validator)
  , m_validator(validator)
  , m_credit(DEFAULT_CREDIT)
  , m_canBePrefix(DEFAULT_CANBE_PREFIX)
  , m_maxTimeout(MAX_TIMEOUT)
  , m_noEndTimeout(NOEND_TIMEOUT)
  , m_interestLifetime(DEFAULT_INTEREST_LIFETIME)
  , m_clusterNodePrefix(clusterNodePrefix)
  , m_clusterPrefix(clusterPrefix)
  , m_repoPrefix(Name(clusterNodePrefix).append(clusterPrefix))
  , m_keySpaceHandle(keySpaceHandle)
{
  dispatcher.addControlCommand<RepoCommandParameter>(ndn::PartialName("insert"),
    makeAuthorization(),
    std::bind(&WriteHandle::validateParameters<InsertCommand>, this, _1),
    std::bind(&WriteHandle::handleInsertCommand, this, _1, _2, _3, _4));

  dispatcher.addControlCommand<RepoCommandParameter>(ndn::PartialName("insert check"),
    makeAuthorization(),
    std::bind(&WriteHandle::validateParameters<InsertCheckCommand>, this, _1),
    std::bind(&WriteHandle::handleCheckCommand, this, _1, _2, _3, _4));

  ndn::InterestFilter filterGet = Name(m_repoPrefix).append("write-info");
  face.setInterestFilter(filterGet,
                           std::bind(&WriteHandle::handleInfoCommand, this, _1, _2),
                           std::bind(&WriteHandle::onRegisterFailed, this, _1, _2));
}

void
WriteHandle::deleteProcess(ProcessId processId)
{
  m_processes.erase(processId);
}

void
WriteHandle::handleInsertCommand(const Name& prefix, const Interest& interest,
                                 const ndn::mgmt::ControlParameters& parameter,
                                 const ndn::mgmt::CommandContinuation& done)
{
  RepoCommandParameter* repoParameter =
    dynamic_cast<RepoCommandParameter*>(const_cast<ndn::mgmt::ControlParameters*>(&parameter));

  auto difsKey = repoParameter->getName().at(-1).toUri();

  auto hash = Manifest::getHash("/" + difsKey);
  auto repo = m_keySpaceHandle.getManifestStorage(hash);
  RepoCommandParameter parameters;
  parameters.setName(hash);
  parameters.setProcessId(repoParameter->getProcessId());

  Interest findInterest = util::generateCommandInterest(repo, "find", parameters, m_interestLifetime);

  auto nackHandler = std::bind([](const Interest& interest) {}, _1);

  face.expressInterest(
    findInterest,
    std::bind(&WriteHandle::onFindResponse, this, _1, _2, interest, *repoParameter, done),
    nackHandler,
    nackHandler);
}

void
WriteHandle::onFindResponse(
    const Interest &findInterest, const Data &findData,
    const Interest &origInterest, const RepoCommandParameter& repoParameter, const ndn::mgmt::CommandContinuation &done)
{
  auto content = findData.getContent();
  if (content.value_size() > 0) {  // Manifest found, cannot insert
    done(negativeReply("Manifest already exists", 403));
    return;
  }

  if (repoParameter.hasStartBlockId() || repoParameter.hasEndBlockId()) {
    NDN_LOG_DEBUG("Process segmented insert");
    processSegmentedInsertCommand(origInterest, repoParameter, done);
  }
  else {
    processSingleInsertCommand(origInterest, repoParameter, done);
  }
  if (repoParameter.hasInterestLifetime())
    m_interestLifetime = repoParameter.getInterestLifetime();
}

void
WriteHandle::onData(const Interest& interest, const Data& data, ProcessId processId)
{
  m_validator.validate(data,
                       std::bind(&WriteHandle::onDataValidated, this, interest, _1, processId),
                       [](const Data& data, const ValidationError& error){NDN_LOG_ERROR("Error: " << error);});
}

void
WriteHandle::onDataValidated(const Interest& interest, const Data& data, ProcessId processId)
{
  if (m_processes.count(processId) == 0) {
    return;
  }

  ProcessInfo& process = m_processes[processId];

  auto content = data.getContent();
  std::string json(
    content.value_begin(),
    content.value_end()
  );

  Manifest manifest = Manifest::fromInfoJson(json);

  process.startBlockId = manifest.getStartBlockId();
  process.endBlockId = manifest.getEndBlockId();
  std::string name = manifest.getName();

  unsigned int i = name.rfind("/");
  std::string difsKey = name.substr(i + 1);
  process.name = difsKey;
  process.repo = m_repoPrefix;
	
  if (!process.manifestSent) {
    process.manifestSent = true;
    writeManifest(processId);
  }

  RepoCommandParameter parameter;
  parameter.setName(manifest.getName());
  parameter.setStartBlockId(0);
  if (!interest.getForwardingHint().empty())
    parameter.setNodePrefix(interest.getForwardingHint());

  segInit(processId, parameter);
}

void
WriteHandle::onTimeout(const Interest& interest, ProcessId processId)
{
  NDN_LOG_DEBUG("Timeout" << std::endl);
  m_processes.erase(processId);
}

void
WriteHandle::processSingleInsertCommand(const Interest& interest, const RepoCommandParameter& parameter,
                                        const ndn::mgmt::CommandContinuation& done)
{
  ProcessId processId = ndn::random::generateWord64();
  NDN_LOG_DEBUG("Insert(manifest) command processId: " << processId << " " << parameter.getName());

  ProcessInfo& process = m_processes[processId];

  RepoCommandResponse& response = process.response;
  response.setCode(100);
  response.setProcessId(processId);
  response.setInsertNum(0);
  response.setBody(response.wireEncode());
  done(response);

  response.setCode(300);
  Interest fetchInterest(parameter.getName());
  fetchInterest.setCanBePrefix(m_canBePrefix);
  fetchInterest.setInterestLifetime(m_interestLifetime);
  fetchInterest.setMustBeFresh(true);
  if (parameter.hasNodePrefix())
    fetchInterest.setForwardingHint(parameter.getNodePrefix());
  face.expressInterest(fetchInterest,
                       std::bind(&WriteHandle::onData, this, _1, _2, processId),
                       std::bind(&WriteHandle::onTimeout, this, _1, processId), // Nack
                       std::bind(&WriteHandle::onTimeout, this, _1, processId));
}

void
WriteHandle::segInit(ProcessId processId, const RepoCommandParameter& parameter)
{
  // use HCSegmentFetcher to send fetch interest.
  ProcessInfo& process = m_processes[processId];
  Name name = parameter.getName();
  SegmentNo startBlockId = parameter.getStartBlockId();

  uint64_t initialCredit = m_credit;

  if (parameter.hasEndBlockId()) {
    initialCredit =
      std::min(initialCredit, parameter.getEndBlockId() - parameter.getStartBlockId() + 1);
    process.endBlockId = parameter.getEndBlockId();

  }
  else {
    // set noEndTimeout timer
    process.noEndTime = ndn::time::steady_clock::now() +
                        m_noEndTimeout;
  }

  Name fetchName = name;
  SegmentNo segment = startBlockId;
  fetchName.appendSegment(segment);

  Interest interest(fetchName);
  interest.setCanBePrefix(m_canBePrefix);
  interest.setMustBeFresh(true);

  if (parameter.hasNodePrefix()) {
    interest.setForwardingHint(parameter.getNodePrefix());
  }

  ndn::util::SegmentFetcher::Options options;
  options.initCwnd = initialCredit;
  options.interestLifetime = m_interestLifetime;
  options.maxTimeout = m_maxTimeout;
  
  std::shared_ptr<ndn::util::HCSegmentFetcher> hc_fetcher;
  auto hcFetcher = hc_fetcher->start(face, interest, m_validator, options);
  hcFetcher->onError.connect([] (uint32_t errorCode, const std::string& errorMsg)
                           {NDN_LOG_ERROR("Error: " << errorMsg);});
  hcFetcher->afterSegmentValidated.connect([this, hcFetcher, processId] (const Data& data)
                                         {onSegmentData(*hcFetcher, data, processId);});
  hcFetcher->afterSegmentTimedOut.connect([this, hcFetcher, processId] ()
                                        {onSegmentTimeout(*hcFetcher, processId);});
}

void
WriteHandle::onSegmentData(ndn::util::HCSegmentFetcher& fetcher, const Data& data, ProcessId processId)
{
  auto it = m_processes.find(processId);
  if (it == m_processes.end()) {
    fetcher.stop();
    return;
  }

  RepoCommandResponse& response = it->second.response;

  //insert data
  if (storageHandle.insertData(data)) {
    response.setInsertNum(response.getInsertNum() + 1);
  }

  ProcessInfo& process = m_processes[processId];

  //read whether notime timeout
  if (!response.hasEndBlockId()) {

    ndn::time::steady_clock::TimePoint& noEndTime = process.noEndTime;
    ndn::time::steady_clock::TimePoint now = ndn::time::steady_clock::now();

    if (now > noEndTime) {
      NDN_LOG_DEBUG("noEndtimeout: " << processId);
      //StatusCode should be refreshed as 405
      response.setCode(405);
      //schedule a delete event
      deferredDeleteProcess(processId);
      fetcher.stop();
      return;
    }
  }

  //read whether this process has total ends, if ends, remove control info from the maps
  if (response.hasEndBlockId()) {
    it->second.endBlockId = response.getEndBlockId();
    uint64_t nSegments = response.getEndBlockId() - response.getStartBlockId() + 1;
    if (response.getInsertNum() >= nSegments) {
      //All the data has been inserted, StatusCode is refreshed as 200
      response.setCode(200);
      deferredDeleteProcess(processId);
      fetcher.stop();
      return;
    }
  }
}

void
WriteHandle::onSegmentTimeout(ndn::util::HCSegmentFetcher& fetcher, ProcessId processId)
{
  NDN_LOG_DEBUG("SegTimeout");
  if (m_processes.count(processId) == 0) {
    fetcher.stop();
    return;
  }
}

void
WriteHandle::processSegmentedInsertCommand(const Interest& interest, const RepoCommandParameter& parameter,
                                           const ndn::mgmt::CommandContinuation& done)
{
  if (parameter.hasEndBlockId()) {
    //normal fetch segment
    SegmentNo startBlockId = parameter.hasStartBlockId() ? parameter.getStartBlockId() : 0;
    SegmentNo endBlockId = parameter.getEndBlockId();
    if (startBlockId > endBlockId) {
      done(negativeReply("Malformed Command", 403));
      return;
    }

    ProcessId processId = ndn::random::generateWord64();
    NDN_LOG_DEBUG("Insert command processId: " << processId);
    ProcessInfo& process = m_processes[processId];
    RepoCommandResponse& response = process.response;
    response.setCode(100);
    response.setProcessId(processId);
    response.setInsertNum(0);
    response.setStartBlockId(startBlockId);
    response.setEndBlockId(endBlockId);
    response.setBody(response.wireEncode());
    done(response);

    //300 means data fetching is in progress
    response.setCode(300);

    segInit(processId, parameter);
  }
  else {
    //no EndBlockId, so fetch FinalBlockId in data, if timeout, stop
    ProcessId processId = ndn::random::generateWord64();
    ProcessInfo& process = m_processes[processId];
    RepoCommandResponse& response = process.response;
    response.setCode(100);
    response.setProcessId(processId);
    response.setInsertNum(0);
    response.setStartBlockId(parameter.getStartBlockId());
    response.setBody(response.wireEncode());
    done(response);

    //300 means data fetching is in progress
    response.setCode(300);

    segInit(processId, parameter);
  }
}

void
WriteHandle::handleCheckCommand(const Name& prefix, const Interest& interest,
                                const ndn::mgmt::ControlParameters& parameter,
                                const ndn::mgmt::CommandContinuation& done)
{
  const RepoCommandParameter& repoParameter = dynamic_cast<const RepoCommandParameter&>(parameter);

  //check whether this process exists
  ProcessId processId = repoParameter.getProcessId();
  if (m_processes.count(processId) == 0) {
    NDN_LOG_DEBUG("no such processId: " << processId);
    done(negativeReply("No such this process is in progress", 404));
    return;
  }

  NDN_LOG_DEBUG("Check command processId: " << processId);
  ProcessInfo& process = m_processes[processId];

  RepoCommandResponse& response = process.response;

  //read if noEndtimeout
  if (!response.hasEndBlockId()) {
    extendNoEndTime(process);
    done(response);
    return;
  }
  else {
    done(response);
  }
}

void
WriteHandle::handleInfoCommand(const Name& prefix, const Interest& interest)
{
  RepoCommandParameter repoParameter;
  extractParameter(interest, prefix, repoParameter);

  ProcessId processId = repoParameter.getProcessId();
  if (m_processes.count(processId) == 0) {
    NDN_LOG_DEBUG("no such processId: " << processId);
    negativeReply("No such this process is in process", 404);
    return;
  }

  NDN_LOG_DEBUG("Got info command: " << processId);

  ProcessInfo& process = m_processes[processId];

  Manifest manifest = *process.manifest;

  auto json = manifest.toJson();
  NDN_LOG_DEBUG("Manifest: " << json << " Interest: " << interest.toUri());
  reply(interest, json);
}

void
WriteHandle::onRegisterFailed(const Name& prefix, const std::string& reason)
{
  NDN_LOG_ERROR("ERROR: Failed to register prefix in local hub's daemon");
  face.shutdown();
}

void
WriteHandle::deferredDeleteProcess(ProcessId processId)
{
  scheduler.schedule(PROCESS_DELETE_TIME, [=] { deleteProcess(processId); });
}

void
WriteHandle::extendNoEndTime(ProcessInfo& process)
{
  auto& noEndTime = process.noEndTime;
  auto now = ndn::time::steady_clock::now();
  RepoCommandResponse& response = process.response;
  if (now > noEndTime) {
    response.setCode(405);
    return;
  }

  process.noEndTime = ndn::time::steady_clock::now() + m_noEndTimeout;
}

RepoCommandResponse
WriteHandle::negativeReply(std::string text, int statusCode)
{
  RepoCommandResponse response(statusCode, text);
  response.setBody(response.wireEncode());

  return response;
}

void
WriteHandle::writeManifest(const ProcessId& processId)
{
  ProcessInfo& process = m_processes[processId];

  std::string repo = process.repo.toUri();
  std::string name = process.name.toUri();
  int startBlockId = process.startBlockId;
  int endBlockId = process.endBlockId;

  Manifest manifest(name, startBlockId, endBlockId);
  manifest.appendRepo(repo, startBlockId, endBlockId);
  auto hash = manifest.getHash();
  NDN_LOG_DEBUG("Manifest name: " << name << " hash: " << hash << " end: " << endBlockId);

  // Save it for later info command
  process.manifest = std::make_shared<Manifest>(manifest);

  auto manifestRepo = m_keySpaceHandle.getManifestStorage(hash);

  NDN_LOG_DEBUG("Using manifest repo: " << manifestRepo);

  RepoCommandParameter parameters;
  parameters.setName(hash);
  parameters.setClusterPrefix(ndn::encoding::makeBinaryBlock(tlv::ClusterPrefix, m_clusterNodePrefix.toUri().c_str(), m_clusterNodePrefix.toUri().length()));

  parameters.setProcessId(processId);
  NDN_LOG_DEBUG("Write manifest for pid " << processId);
  Interest createInterest = util::generateCommandInterest(
    manifestRepo, "create", parameters, m_interestLifetime);
  
  face.expressInterest(
    createInterest,
    std::bind(&WriteHandle::onCreateCommandResponse, this, _1, _2, processId),
    std::bind(&WriteHandle::onCreateCommandTimeout, this, _1, processId),
    std::bind(&WriteHandle::onCreateCommandTimeout, this, _1, processId));
}

void
WriteHandle::onCreateCommandResponse(
  const Interest& interest, const Data& data, const ProcessId& processId)
{
  NDN_LOG_DEBUG("Got create command response");
}

void
WriteHandle::onCreateCommandTimeout(
  const Interest& interest, const ProcessId& processId)
{
  NDN_LOG_ERROR("Create timeout");
}

} // namespace repo
