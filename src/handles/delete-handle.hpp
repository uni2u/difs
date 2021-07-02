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

#ifndef REPO_HANDLES_DELETE_HANDLE_HPP
#define REPO_HANDLES_DELETE_HANDLE_HPP

#include "command-base-handle.hpp"
#include "keyspace-handle.hpp"

#include <ndn-cxx/mgmt/dispatcher.hpp>

namespace repo {

class DeleteHandle : public CommandBaseHandle
{

public:
  class Error : public CommandBaseHandle::Error
  {
  public:
    explicit
    Error(const std::string& what)
      : CommandBaseHandle::Error(what)
    {
    }
  };

private:
  struct ProcessInfo
  {
    Interest interest;
    std::list<Manifest::Repo> repos;
    ndn::Name name;
    std::string hash;
  };

public:
  DeleteHandle(Face& face, KeySpaceHandle& keySpaceHandle, RepoStorage& storageHandle,
               ndn::mgmt::Dispatcher& dispatcher, Scheduler& scheduler, Validator& validator,
               ndn::Name const& clusterNodePrefix, std::string clusterPrefix);

private:
  void
  handleDeleteCommand(const Name& prefix, const Interest& interest,
                      const ndn::mgmt::ControlParameters& parameters,
                      const ndn::mgmt::CommandContinuation& done);

  void
  onDeleteManifestCommandResponse(const Interest& interest, const Data& data,
                                                const ndn::mgmt::CommandContinuation& done,
                                                const RepoCommandParameter& repoParameter,
                                                const ProcessId processid);

  void
  onTimeout(const Interest& interest, const ProcessId processId);

  void
  handleDeleteManifestCommand(const Name& prefix, const Interest& interest);

  void
  handleOnlyDeleteManifestCommand(const Name& prefix, const Interest& interest);

  void
  deleteData(const RepoCommandParameter repoParameter, ProcessId processId);

  void
  onDeleteDataCommandResponse(const Interest& interest, const Data& data,
                              const RepoCommandParameter& parameters,
                              const ProcessId processId);

  void
  handleDeleteDataCommand(const Name& prefix, const Interest& interest);

  RepoCommandResponse
  positiveReply(const Interest& interest, const RepoCommandParameter& parameter,
                uint64_t statusCode, uint64_t nDeletedData) const;

  RepoCommandResponse
  negativeReply(const Interest& interest, uint64_t statusCode, const std::string text) const;

  void
  onRegisterFailed(const Name& prefix, const std::string& reason);

private:
  std::map<ProcessId, ProcessInfo> m_processes;

  ndn::time::milliseconds m_interestLifetime;
  KeySpaceHandle& m_keySpaceHandle;
  ndn::Name m_repoPrefix;
};

} // namespace repo

#endif // REPO_HANDLES_DELETE_HANDLE_HPP
