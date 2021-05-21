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

#include "manifest-handle.hpp"

#include <ndn-cxx/util/logger.hpp>
#include <ndn-cxx/util/random.hpp>

#include "manifest/manifest.hpp"
#include "util.hpp"

namespace repo {

NDN_LOG_INIT(repo.ManifestHandle);

static const int DEFAULT_CREDIT = 12;
static const bool DEFAULT_CANBE_PREFIX = false;
static const milliseconds MAX_TIMEOUT(60_s);
static const milliseconds NOEND_TIMEOUT(10000_ms);
static const milliseconds PROCESS_DELETE_TIME(10000_ms);
static const milliseconds DEFAULT_INTEREST_LIFETIME(4000_ms);

ManifestHandle::ManifestHandle(Face& face, RepoStorage& storageHandle, ndn::mgmt::Dispatcher& dispatcher,
                         Scheduler& scheduler, Validator& validator,
                         ndn::Name const& clusterPrefix, const int clusterId)
  : CommandBaseHandle(face, storageHandle, scheduler, validator)
  , m_validator(validator)
  , m_credit(DEFAULT_CREDIT)
  , m_canBePrefix(DEFAULT_CANBE_PREFIX)
  , m_maxTimeout(MAX_TIMEOUT)
  , m_noEndTimeout(NOEND_TIMEOUT)
  , m_interestLifetime(DEFAULT_INTEREST_LIFETIME)
  , m_clusterPrefix(clusterPrefix)
  , m_clusterId(clusterId)
  , m_repoPrefix(Name(clusterPrefix).append(std::to_string(clusterId)))
{
  dispatcher.addControlCommand<RepoCommandParameter>(
    ndn::PartialName(std::to_string(clusterId)).append("create"),
    makeAuthorization(),
    std::bind(&ManifestHandle::validateParameters<CreateCommand>, this, _1),
    std::bind(&ManifestHandle::handleCreateCommand, this, _1, _2, _3, _4));

  // dispatcher.addControlCommand<RepoCommandParameter>(
  //   ndn::PartialName(std::to_string(clusterId)).append("find"),
  //   makeAuthorization(),
  //   std::bind(&ManifestHandle::validateParameters<FindCommand>, this, _1),
  //   std::bind(&ManifestHandle::handleFindCommand, this, _1, _2, _3, _4));

  ndn::InterestFilter filterFind = Name(m_repoPrefix).append("find");
  NDN_LOG_DEBUG(m_repoPrefix << " Listening " << filterFind);
  face.setInterestFilter(filterFind,
                           std::bind(&ManifestHandle::handleFindCommand, this, _1, _2),
                           std::bind(&ManifestHandle::onRegisterFailed, this, _1, _2));
}

void
ManifestHandle::deleteProcess(ProcessId processId)
{
  m_processes.erase(processId);
}

void
ManifestHandle::handleCreateCommand(const Name& prefix, const Interest& interest,
                                 const ndn::mgmt::ControlParameters& parameter,
                                 const ndn::mgmt::CommandContinuation& done)
{
  RepoCommandParameter* repoParameter =
    dynamic_cast<RepoCommandParameter*>(const_cast<ndn::mgmt::ControlParameters*>(&parameter));

  // auto processId = repoParameter->getProcessId();
  // ProcessInfo& process = m_processes[processId];

  auto repo = m_clusterPrefix;
  auto hash = repoParameter->getName();
  auto clusterId = repoParameter->getClusterId();

  NDN_LOG_DEBUG("Got create hash " << hash << " from " << clusterId);

  ProcessId processId = repoParameter->getProcessId();

  ProcessInfo& process = m_processes[processId];

  RepoCommandResponse& response = process.response;
  response.setCode(100);
  response.setProcessId(processId);
  response.setInsertNum(0);
  response.setBody(response.wireEncode());
  done(response);

  RepoCommandParameter parameters;
  parameters.setName(hash);
  parameters.setProcessId(processId);

  auto commandPath = Name(m_clusterPrefix)
    .append(std::to_string(clusterId))
    .append("info")
    .append(parameters.wireEncode());
  NDN_LOG_DEBUG("request " << commandPath);
  Interest fetchInterest(commandPath);
  fetchInterest.setInterestLifetime(m_interestLifetime);
  // fetchInterest.setMustBeFresh(true);

  face.expressInterest(fetchInterest,
                       std::bind(&ManifestHandle::onData, this, _1, _2, processId),
                       std::bind(&ManifestHandle::onTimeout, this, _1, processId), // Nack
                       std::bind(&ManifestHandle::onTimeout, this, _1, processId));

  if (repoParameter->hasInterestLifetime())
    m_interestLifetime = repoParameter->getInterestLifetime();
}

void
ManifestHandle::onData(const Interest& interest, const Data& data, ProcessId processId)
{
  m_validator.validate(data,
                       std::bind(&ManifestHandle::onDataValidated, this, interest, _1, processId),
                       [](const Data& data, const ValidationError& error){NDN_LOG_ERROR("Error: " << error);});
}

void
ManifestHandle::onDataValidated(const Interest& interest, const Data& data, ProcessId processId)
{
  if (m_processes.count(processId) == 0) {
    return;
  }

  ProcessInfo& process = m_processes[processId];
  RepoCommandResponse& response = process.response;

  if (response.getInsertNum() == 0) {
    auto content = data.getContent();
    std::string json(
      content.value_begin(),
      content.value_end());
    auto manifest = Manifest::fromJson(json);
    NDN_LOG_DEBUG("Saving manifest");
    storageHandle.insertManifest(manifest);
    response.setInsertNum(1);
  }

  deferredDeleteProcess(processId);
}

void
ManifestHandle::onTimeout(const Interest& interest, ProcessId processId)
{
  NDN_LOG_DEBUG("Timeout" << std::endl);
  m_processes.erase(processId);
}

void
ManifestHandle::processSingleCreateCommand(const Interest& interest, RepoCommandParameter& parameter,
                                        const ndn::mgmt::CommandContinuation& done)
{
}

void
ManifestHandle::onSegmentTimeout(ndn::util::HCSegmentFetcher& fetcher, ProcessId processId)
{
  NDN_LOG_DEBUG("SegTimeout");
  if (m_processes.count(processId) == 0) {
    fetcher.stop();
    return;
  }
}

void
ManifestHandle::handleFindCommand(const Name& prefix, const Interest& interest)
{
  RepoCommandParameter repoParameter;
  try {
    extractParameter(interest, prefix, repoParameter);
  } catch (RepoCommandParameter::Error&) {
    negativeReply(interest, "command parameter malformed", 403);
  }

  auto hash = repoParameter.getName().toUri();
  hash = hash.substr(1, hash.length() - 1);  // Remove prepended /
  NDN_LOG_DEBUG("Got find interest " << hash);
  auto manifest = storageHandle.readManifest(hash);
  if (manifest != nullptr) {
    auto json = manifest->toJson();
    reply(interest, json);
  } else {
    NDN_LOG_DEBUG("Cannot found manifest " << hash);
    reply(interest, "");
  }

}

void
ManifestHandle::onRegisterFailed(const Name& prefix, const std::string& reason)
{
  NDN_LOG_ERROR("ERROR: Failed to register prefix in local hub's daemon");
  face.shutdown();
}

void
ManifestHandle::deferredDeleteProcess(ProcessId processId)
{
  scheduler.schedule(PROCESS_DELETE_TIME, [=] { deleteProcess(processId); });
}

void
ManifestHandle::extendNoEndTime(ProcessInfo& process)
{
  auto& noEndTime = process.noEndTime;
  auto now = ndn::time::steady_clock::now();
  RepoCommandResponse& response = process.response;
  if (now > noEndTime) {
    response.setCode(405);
    return;
  }

  //extends noEndTime
  process.noEndTime = ndn::time::steady_clock::now() + m_noEndTimeout;
}

} // namespace repo
