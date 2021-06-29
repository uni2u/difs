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

#ifndef REPO_HANDLES_KEYSPACE_HANDLE_HPP
#define REPO_HANDLES_KEYSPACE_HANDLE_HPP

#include "command-base-handle.hpp"

#include <ndn-cxx/mgmt/dispatcher.hpp>

namespace repo {

/**
 * @brief WriteHandle provides basic credit based congestion control.
 *
 * First repo sends interests of credit number and then credit will be 0.
 *
 * If a data comes, credit++ and sends a interest then credit--.
 *
 * If the interest timeout, repo will retry and send interest in retrytimes.
 *
 * If one interest timeout beyond retrytimes, the fetching process will terminate.
 *
 * Another case is that if command will insert segmented data without EndBlockId.
 *
 * The repo will keep fetching data in noendTimeout time.
 *
 * If data returns with FinalBlockId, this detecting timeout process will terminate.
 *
 * If client sends a insert check command, the noendTimeout timer will be set to 0.
 *
 * If repo cannot get FinalBlockId in noendTimeout time, the fetching process will terminate.
 */
class KeySpaceHandle : public CommandBaseHandle
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

public:
  KeySpaceHandle(Face& face, RepoStorage& storageHandle,
              ndn::mgmt::Dispatcher& dispatcher, Scheduler& scheduler,
              Validator& validator,
              ndn::Name const& clusterPrefix, ndn::Name const& managerPrefix, const int clusterId, 
              std::string clusterType, std::string from);

private:
  /**
  * @brief Information of insert process including variables for response
  *        and credit based flow control
  */
  struct ProcessInfo
  {
    RepoCommandResponse response;
    std::queue<SegmentNo> nextSegmentQueue;  ///< queue of waiting segment
                                        ///  to be sent when having credits
    SegmentNo nextSegment;  ///< last segment put into the nextSegmentQueue
    std::map<SegmentNo, int> retryCounts;  ///< to store retrying times of timeout segment
    int credit;  ///< congestion control credits of process

    /**
     * @brief the latest time point at which EndBlockId must be determined
     *
     * Segmented fetch process will terminate if EndBlockId cannot be
     * determined before this time point.
     * It is initialized to now()+noEndTimeout when segmented fetch process begins,
     * and reset to now()+noEndTimeout each time an insert status check command is processed.
     */
    ndn::time::steady_clock::TimePoint noEndTime;

    ndn::Name repo;
    ndn::Name name;
    int startBlockId;
    int endBlockId;
    std::shared_ptr<Manifest> manifest;

    bool manifestSent = false;
  };

private:
  void
  handleRingInfoCommand(const Name& prefix, const Interest& interest);

  void
  handleVersionCommand(const Name& prefix, const Interest& interest);

  void
  handleAddCommand(const Name& prefix, const Interest& interest,
                   const ndn::mgmt::ControlParameters& parameter,
                   const ndn::mgmt::CommandContinuation& done);

  void
  handleDeleteCommand(const Name& prefix, const Interest& interest,
                      const ndn::mgmt::ControlParameters& parameter,
                      const ndn::mgmt::CommandContinuation& done);

  void
  handleFetchCommand(const Name& prefix, const Interest& interest);

  void
  handleCoordinationCommand(const Name &prefix, const Interest &interest,
                            const ndn::mgmt::ControlParameters &parameter);

  void
  handleManifestListCommand(const Name& prefix, const Interest& interest);

  void
  handleManifestCommand(const Name& prefix, const Interest& interest);

  void
  onFetchCommand(std::string versionNum);

  void
  onFetchCommandResponse(const Interest& interest, const Data& data);

  void
  onFetchCommandTimeout(const Interest& interest);

  void
  onVersionCommand(); 

  void
  onVersionCommandResponse(const Interest& interest, const Data& data);

  void
  onVersionCommandTimeout(const Interest& interest);

  void
  onManifestListCommand();

  void
  onManifestListCommandResponse(const Interest& interest, const Data& data);

  void
  onManifestListCommandTimeout(const Interest& interest);

  void
  onManifestCommand(std::string manifestName);

  void
  onManifestCommandResponse(const Interest& interest, const Data& data);

  void
  onManifestCommandTimeout(const Interest& interest);

  void
  onCoordinationCommand();

  void
  onCoordinationCommandResponse(const Interest& interest, const Data& data);

  void
  onCoordinationCommandTimeout(const Interest& interest);

  void
  onRegisterFailed(const Name& prefix, const std::string& reason);

public:
  ndn::Name
  getManifestStorage(const std::string hash);

private:
  std::map<ProcessId, ProcessInfo> m_processes;

  int m_credit;
  bool m_canBePrefix;
  ndn::time::milliseconds m_maxTimeout;
  ndn::time::milliseconds m_noEndTimeout;
  ndn::time::milliseconds m_interestLifetime;

  ndn::Name m_selfRepo;
  ndn::Name m_clusterPrefix;
  ndn::Name m_managerPrefix;
  int m_clusterId;
  std::string m_clusterType;
  ndn::Name m_repoPrefix;
  uint64_t m_version;
  std::string m_versionNum;
  std::string m_keySpaceFile, m_manifestList;
  std::string m_start, m_end;
  std::string m_from, m_to;
};

}

#endif