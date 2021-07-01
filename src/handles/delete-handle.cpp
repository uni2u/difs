/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018, Regents of the University of California.
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

#include "delete-handle.hpp"
#include "../util.hpp"

#include <ndn-cxx/util/logger.hpp>
#include <ndn-cxx/util/random.hpp>

namespace repo {

NDN_LOG_INIT(repo.DeleteHandle);

static const milliseconds DEFAULT_INTEREST_LIFETIME(4000);

DeleteHandle::DeleteHandle(Face& face, KeySpaceHandle& keySpaceHandle, RepoStorage& storageHandle,
                           ndn::mgmt::Dispatcher& dispatcher, Scheduler& scheduler,
                           Validator& validator, ndn::Name const& clusterNodePrefix, std::string clusterPrefix)
  : CommandBaseHandle(face, storageHandle, scheduler, validator)
  , m_interestLifetime(DEFAULT_INTEREST_LIFETIME)
  , m_keySpaceHandle(keySpaceHandle)
  , m_repoPrefix(Name(clusterNodePrefix).append(clusterPrefix))
{
  ndn::InterestFilter filterDeleteManifest = Name(m_repoPrefix).append("delete-manifest");
  NDN_LOG_DEBUG(m_repoPrefix << " Listening " << filterDeleteManifest);
  face.setInterestFilter(filterDeleteManifest,
                           std::bind(&DeleteHandle::handleDeleteManifestCommand, this, _1, _2),
                           std::bind(&DeleteHandle::onRegisterFailed, this, _1, _2));

  ndn::InterestFilter filterDeleteData = Name(m_repoPrefix).append("delete-data");
  NDN_LOG_DEBUG(m_repoPrefix << " Listening " << filterDeleteData);
  face.setInterestFilter(filterDeleteData,
                           std::bind(&DeleteHandle::handleDeleteDataCommand, this, _1, _2),
                           std::bind(&DeleteHandle::onRegisterFailed, this, _1, _2));

  dispatcher.addControlCommand<RepoCommandParameter>(ndn::PartialName("delete"),
    makeAuthorization(),
    std::bind(&DeleteHandle::validateParameters<DeleteCommand>, this, _1),
    std::bind(&DeleteHandle::handleDeleteCommand, this, _1, _2, _3, _4));

  // ndn::InterestFilter filterDelete = Name(m_repoPrefix).append("delete");
  // NDN_LOG_DEBUG(m_repoPrefix << " Listening " << filterDelete);
  // face.setInterestFilter(filterDelete,
  //                          std::bind(&DeleteHandle::handleDeleteCommand, this, _1, _2),
  //                          std::bind(&DeleteHandle::onRegisterFailed, this, _1, _2));

  // dispatcher.addControlCommand<RepoCommandParameter>(
  //   ndn::PartialName(clusterPrefix).append("delete manifest"),
  //   makeAuthorization(),
  //   std::bind(&DeleteHandle::validateParameters<DeleteManifestCommand>, this, _1),
  //   std::bind(&DeleteHandle::handleDeleteManifestCommand, this, _1, _2, _3, _4));

  // dispatcher.addControlCommand<RepoCommandParameter>(
  //   ndn::PartialName(clusterPrefix).append("delete data"),
  //   makeAuthorization(),
  //   std::bind(&DeleteHandle::validateParameters<DeleteDataCommand>, this, _1),
  //   std::bind(&DeleteHandle::handleDeleteDataCommand, this, _1, _2, _3, _4));
}

void
DeleteHandle::handleDeleteCommand(const Name& prefix, const Interest& interest,
                                  const ndn::mgmt::ControlParameters& parameter,
                                  const ndn::mgmt::CommandContinuation& done)
{
  const RepoCommandParameter& repoParameter = dynamic_cast<const RepoCommandParameter&>(parameter);

  NDN_LOG_DEBUG("Got delete command " << repoParameter.getName());

  auto name = repoParameter.getName().toUri();
  auto hash = Manifest::getHash(name);

  ProcessId processId = repoParameter.getProcessId();
  ProcessInfo& process = m_processes[processId];
  process.interest = interest;

  RepoCommandParameter parameters;
  parameters.setName(hash);
  parameters.setProcessId(processId);

  auto repo = m_keySpaceHandle.getManifestStorage(hash);
  Interest deleteManifestInterest = util::generateCommandInterest(
    repo, "delete-manifest", parameters, m_interestLifetime);

  NDN_LOG_DEBUG(deleteManifestInterest);
  
  face.expressInterest(
    deleteManifestInterest,
    std::bind(&DeleteHandle::onDeleteManifestCommandResponse, this, _1, _2, done, repoParameter, processId),
    std::bind(&DeleteHandle::onTimeout, this, _1, processId),
    std::bind(&DeleteHandle::onTimeout, this, _1, processId));
}

void
DeleteHandle::onDeleteManifestCommandResponse(const Interest& interest, const Data& data,
                                              const ndn::mgmt::CommandContinuation& done,
                                              const RepoCommandParameter& repoParameter,
                                              const ProcessId processId)
{
  RepoCommandResponse response(data.getContent().blockFromValue());
  NDN_LOG_DEBUG("Got delete manifest response " << response.getCode());
  if (response.getCode() == 200) {
    done(positiveReply(interest, repoParameter, 200, 1));
  } else {
    done(negativeReply(interest, 404, "Manifest not found"));
  }

  m_processes.erase(processId);
}

void
DeleteHandle::onTimeout(const Interest& interest, const ProcessId processId)
{
  NDN_LOG_DEBUG("Timeout");
  auto prevInterest = m_processes[processId].interest;
  reply(interest, negativeReply(prevInterest, 405, "Deletion Failed"));
}

void
DeleteHandle::handleDeleteManifestCommand(const Name& prefix, const Interest& interest)
{
  RepoCommandParameter repoParameter;
  extractParameter(interest, prefix, repoParameter);

  auto hash = repoParameter.getName().toUri();
  hash = hash.substr(1, hash.length() - 1);

  NDN_LOG_DEBUG("Got delete manifest " << hash);
  ProcessId processId = ndn::random::generateWord64();
  ProcessInfo& process = m_processes[processId];

  process.interest = interest;

  auto manifest = storageHandle.readManifest(hash);
  if (manifest == nullptr) {
    NDN_LOG_DEBUG("Manifest not found");
    reply(interest, negativeReply(interest, 405, "Manifest not found"));
    return;
  }

  process.repos = manifest->getRepos();
  process.name = manifest->getName();
  process.hash = hash;

  deleteData(repoParameter, processId);
}

void
DeleteHandle::deleteData(const RepoCommandParameter repoParameter, ProcessId processId)
{
  ProcessInfo& process = m_processes[processId];

  Manifest::Repo repo = process.repos.front();
  process.repos.pop_front();

  auto newProcessId = ndn::random::generateWord64();

  RepoCommandParameter parameters;
  // /repo/0/data/data/0/%00%00
  auto name = ndn::Name(repo.name);
  name.append("data");
  name.append(process.name);
  parameters.setName(name);
  parameters.setStartBlockId(repo.start);
  parameters.setEndBlockId(repo.end);
  parameters.setProcessId(newProcessId);

  Interest deleteDataInterest = util::generateCommandInterest(
    ndn::Name(repo.name), "delete-data", parameters, m_interestLifetime);

  NDN_LOG_DEBUG(deleteDataInterest);

  face.expressInterest(
    deleteDataInterest,
    std::bind(&DeleteHandle::onDeleteDataCommandResponse, this, _1, _2, repoParameter, processId),
    std::bind(&DeleteHandle::onTimeout, this, _1, processId),
    std::bind(&DeleteHandle::onTimeout, this, _1, processId));
}

void
DeleteHandle::onDeleteDataCommandResponse(const Interest& interest, const Data& data,
                                          const RepoCommandParameter& parameters,
                                          const ProcessId processId)
{
  RepoCommandResponse response(data.getContent().blockFromValue());
  NDN_LOG_DEBUG("Got delete data response " << response.getCode());

  ProcessInfo& process = m_processes[processId];

  if (response.getCode() > 400) {
    reply(interest, negativeReply(process.interest, 405, "Delete data failed"));
    return;
  }

  auto repos = process.repos;

  if (repos.size() == 0) {
    reply(process.interest, positiveReply(process.interest, parameters, 200, 1));
    storageHandle.deleteManifest(process.hash);
    m_processes.erase(processId);
  } else {
    deleteData(parameters, processId);
  }
}

void
DeleteHandle::handleDeleteDataCommand(const Name& prefix, const Interest& interest)
{
  RepoCommandParameter repoParameter;
  extractParameter(interest, prefix, repoParameter);

  if (!repoParameter.hasStartBlockId() || !repoParameter.hasEndBlockId()) {
    reply(interest, negativeReply(interest, 403, "Start/End block id not set"));
    return;
  }

  SegmentNo start = repoParameter.getStartBlockId();
  SegmentNo end = repoParameter.getEndBlockId();

  if (start > end ) {
    reply(interest, negativeReply(interest, 403, "Start block id > End block id"));
    return;
  }

  NDN_LOG_DEBUG("Got delete data " << repoParameter.getName() << " " << start << "~" << end);

  const Name dataName = repoParameter.getName();
  uint64_t nDeletedData = 0;
  for (SegmentNo i = start; i <= end; ++i) {
    Name name = dataName;
    name.appendSegment(i);
    NDN_LOG_DEBUG("Delete data " << name);
    if (storageHandle.deleteData(name)) {
      nDeletedData += 1;
    }
  }

  reply(interest, positiveReply(interest, repoParameter, 200, nDeletedData));
}

RepoCommandResponse
DeleteHandle::positiveReply(const Interest& interest, const RepoCommandParameter& parameter,
                            uint64_t statusCode, uint64_t nDeletedData) const
{
  RepoCommandResponse response(statusCode, "Deletion Successful");

  if (parameter.hasProcessId()) {
    response.setProcessId(parameter.getProcessId());
    response.setDeleteNum(nDeletedData);
    response.setBody(response.wireEncode());
  }
  else {
    response.setCode(403);
    response.setText("Malformed Command");
    response.setBody(response.wireEncode());
  }
  return response;
}

RepoCommandResponse
DeleteHandle::negativeReply(const Interest& interest, uint64_t statusCode, std::string text) const
{
  RepoCommandResponse response(statusCode, text);
  response.setBody(response.wireEncode());
  return response;
}

void
DeleteHandle::onRegisterFailed(const Name& prefix, const std::string& reason)
{
  NDN_LOG_ERROR("ERROR: Failed to register prefix in local hub's daemon");
  face.shutdown();
}

} // namespace repo
