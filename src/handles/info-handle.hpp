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

#ifndef REPO_HANDLES_INFO_HANDLE_HPP
#define REPO_HANDLES_INFO_HANDLE_HPP

#include "command-base-handle.hpp"

#include <ndn-cxx/mgmt/dispatcher.hpp>
#include <ndn-cxx/util/hc-segment-fetcher.hpp>

#include <queue>

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
class InfoHandle : public CommandBaseHandle
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
  InfoHandle(Face& face, RepoStorage& storageHandle,
              Scheduler& scheduler, Validator& validator,
              ndn::Name const& clusterNodePrefix, std::string clusterPrefix);

private:
  void
  handleInfoCommand(const Name& prefix, const Interest& interest);

  void
  onRegisterFailed(const Name& prefix, const std::string& reason);

private:
  ndn::Name m_repoPrefix;
};
}

#endif