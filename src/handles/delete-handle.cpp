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

DeleteHandle::DeleteHandle(Face& face, RepoStorage& storageHandle,
                           ndn::mgmt::Dispatcher& dispatcher, Scheduler& scheduler,
                           Validator& validator,
                           ndn::Name& clusterPrefix, const int clusterSize)
  : CommandBaseHandle(face, storageHandle, scheduler, validator)
  , m_interestLifetime(DEFAULT_INTEREST_LIFETIME)
  , m_clusterPrefix(clusterPrefix)
  , m_clusterSize(clusterSize)
{
  dispatcher.addControlCommand<RepoCommandParameter>(ndn::PartialName("delete"),
    makeAuthorization(),
    std::bind(&DeleteHandle::validateParameters<DeleteCommand>, this, _1),
    std::bind(&DeleteHandle::handleDeleteCommand, this, _1, _2, _3, _4));

  dispatcher.addControlCommand<RepoCommandParameter>(ndn::PartialName("delete manifest"),
    makeAuthorization(),
    std::bind(&DeleteHandle::validateParameters<DeleteManifestCommand>, this, _1),
    std::bind(&DeleteHandle::handleDeleteManifestCommand, this, _1, _2, _3, _4));

  dispatcher.addControlCommand<RepoCommandParameter>(ndn::PartialName("delete data"),
    makeAuthorization(),
    std::bind(&DeleteHandle::validateParameters<DeleteDataCommand>, this, _1),
    std::bind(&DeleteHandle::handleDeleteDataCommand, this, _1, _2, _3, _4));

}

void
DeleteHandle::handleDeleteCommand(const Name& prefix, const Interest& interest,
                                  const ndn::mgmt::ControlParameters& parameter,
                                  const ndn::mgmt::CommandContinuation& done)
{
  const RepoCommandParameter& repoParameter = dynamic_cast<const RepoCommandParameter&>(parameter);

  auto name = repoParameter.getName().toUri();
  auto hash = Manifest::getHash(name);

  ProcessId processId = repoParameter.getProcessId();
  ProcessInfo& process = m_processes[processId];
  process.interest = interest;

  RepoCommandParameter parameters;
  parameters.setName(hash);
  parameters.setProcessId(processId);

  auto repo = Manifest::getManifestStorage(m_clusterPrefix, name, m_clusterSize);
  Interest deleteManifestInterest = util::generateCommandInterest(
    repo, "delete manifest", parameters, m_interestLifetime);
  
  face.expressInterest(
    deleteManifestInterest,
    std::bind(&DeleteHandle::onDeleteManifestCommandResponse, this, _1, _2, done, repoParameter, processId),
    std::bind(&DeleteHandle::onTimeout, this, _1, done, processId),
    std::bind(&DeleteHandle::onTimeout, this, _1, done, processId));
}

void
DeleteHandle::onDeleteManifestCommandResponse(const Interest& interest, const Data& data,
                                              const ndn::mgmt::CommandContinuation& done,
                                              const RepoCommandParameter& repoParameter,
                                              const ProcessId processId)
{
  done(positiveReply(interest, repoParameter, 200, 1));
  m_processes.erase(processId);
}

void
DeleteHandle::onTimeout(const Interest& interest, const ndn::mgmt::CommandContinuation& done, const ProcessId processId)
{
  auto prevInterest = m_processes[processId].interest;
  done(negativeReply(prevInterest, 405, "Deletion Failed"));
}

void
DeleteHandle::handleDeleteManifestCommand(const Name& prefix, const Interest& interest,
                                          const ndn::mgmt::ControlParameters& parameter,
                                          const ndn::mgmt::CommandContinuation& done)
{
  const RepoCommandParameter& repoParameter = dynamic_cast<const RepoCommandParameter&>(parameter);

  auto hash = repoParameter.getName().toUri();
  ProcessId processId = ndn::random::generateWord64();
  ProcessInfo& process = m_processes[processId];

  process.interest = interest;

  auto manifest = storageHandle.readManifest(hash);
  if (manifest == nullptr) {
    done(negativeReply(interest, 405, "Manifest not found"));
    return;
  }

  process.repos = manifest->getRepos();
  process.name = manifest->getName();
  process.hash = hash;

  deleteData(repoParameter, done, processId);
}

void
DeleteHandle::deleteData(const RepoCommandParameter repoParameter, const ndn::mgmt::CommandContinuation& done, ProcessId processId)
{
  ProcessInfo& process = m_processes[processId];

  Manifest::Repo repo = process.repos.front();
  process.repos.pop_front();
  RepoCommandParameter parameters;
  // /repo/0/data/data/0/%00%00
  auto name = ndn::Name(repo.name);
  name.append("data");
  name.append(process.name);
  parameters.setName(name);
  parameters.setStartBlockId(repo.start);
  parameters.setEndBlockId(repo.end);

  Interest deleteDataInterest = util::generateCommandInterest(
    ndn::Name(repo.name), "delete data", parameters, m_interestLifetime);

  face.expressInterest(
    deleteDataInterest,
    std::bind(&DeleteHandle::onDeleteDataCommandResponse, this, _1, _2, repoParameter, done, processId),
    std::bind(&DeleteHandle::onTimeout, this, _1, done, processId),
    std::bind(&DeleteHandle::onTimeout, this, _1, done, processId));
}

void
DeleteHandle::onDeleteDataCommandResponse(const Interest& interest, const Data& data,
                                          const RepoCommandParameter& parameters, const ndn::mgmt::CommandContinuation& done,
                                          const ProcessId processId)
{
  ProcessInfo& process = m_processes[processId];

  auto repos = process.repos;

  if (repos.size() == 0) {
    done(positiveReply(process.interest, parameters, 200, 1));
    storageHandle.deleteManifest(process.hash);
    m_processes.erase(processId);
  } else {
    deleteData(parameters, done, processId);
  }
}

void
DeleteHandle::handleDeleteDataCommand(const Name& prefix, const Interest& interest,
                                      const ndn::mgmt::ControlParameters& parameter,
                                      const ndn::mgmt::CommandContinuation& done)
{
  const RepoCommandParameter& repoParameter = dynamic_cast<const RepoCommandParameter&>(parameter);

  if (!repoParameter.hasStartBlockId() || !repoParameter.hasEndBlockId()) {
    done(negativeReply(interest, 403, "Start/End block id not set"));
    return;
  }

  SegmentNo start = repoParameter.getStartBlockId();
  SegmentNo end = repoParameter.getEndBlockId();

  if (start > end ) {
    done(negativeReply(interest, 403, "Start block id > End block id"));
    return;
  }

  const Name dataName = repoParameter.getName();
  uint64_t nDeletedData = 0;
  for (SegmentNo i = start; i <= end; ++i) {
    Name name = dataName;
    name.appendSegment(i);
    if (storageHandle.deleteData(name)) {
      nDeletedData += 1;
    }
  }

  done(positiveReply(interest, repoParameter, 200, nDeletedData));
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

} // namespace repo
