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

#ifndef REPO_REPO_COMMAND_PARAMETER_HPP
#define REPO_REPO_COMMAND_PARAMETER_HPP

#include "repo-tlv.hpp"

#include <ndn-cxx/encoding/encoding-buffer.hpp>
#include <ndn-cxx/encoding/block-helpers.hpp>
#include <ndn-cxx/mgmt/control-parameters.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/delegation-list.hpp>

namespace repo {

using ndn::Name;
using ndn::Block;
using ndn::EncodingImpl;
using ndn::EncodingEstimator;
using ndn::EncodingBuffer;
using namespace ndn::time;

enum RepoParameterField {
  REPO_PARAMETER_NODE_PREFIX, // Forwardhing Hint
  REPO_PARAMETER_NAME,
  REPO_PARAMETER_FROM,
  REPO_PARAMETER_TO,
  REPO_PARAMETER_START_BLOCK_ID,
  REPO_PARAMETER_END_BLOCK_ID,
  REPO_PARAMETER_PROCESS_ID,
  REPO_PARAMETER_INTEREST_LIFETIME,
  REPO_PARAMETER_CLUSTER_PREFIX,
  REPO_PARAMETER_UBOUND
};

const std::string REPO_PARAMETER_FIELD[REPO_PARAMETER_UBOUND] = {
  "NodePrefix",
  "Name",
  "From",
  "To",
  "StartBlockId",
  "EndBlockId",
  "ProcessId",
  "InterestLifetime",
  "ClusterPrefix"
};

/**
* @brief Class defining abstraction of parameter of command for NDN Repo Protocol
* @sa link https://redmine.named-data.net/projects/repo-ng/wiki/Repo_Protocol_Specification#RepoCommandParameter
**/

class RepoCommandParameter : public ndn::mgmt::ControlParameters
{
public:
  class Error : public ndn::tlv::Error
  {
  public:
    explicit
    Error(const std::string& what)
      : ndn::tlv::Error(what)
    {
    }
  };

  RepoCommandParameter()
    : m_hasFields(REPO_PARAMETER_UBOUND)
  {
  }

  explicit
  RepoCommandParameter(const Block& block)
    : m_hasFields(REPO_PARAMETER_UBOUND)
  {
    wireDecode(block);
  }

  const ndn::DelegationList
  getNodePrefix() const noexcept
  {
    return m_nodePrefix;
  }

  RepoCommandParameter&
  setNodePrefix(ndn::DelegationList delegationList);

  bool
  hasNodePrefix() const 
  {
    return m_hasFields[REPO_PARAMETER_NODE_PREFIX];
  }

  const Name&
  getName() const
  {
    return m_name;
  }

  RepoCommandParameter&
  setName(const Name& name);

  bool
  hasName() const
  {
    return m_hasFields[REPO_PARAMETER_NAME];
  }

  const Block&
  getFrom() const
  {
    return m_from;
  }

  RepoCommandParameter&
  setFrom(const Block& from);

  bool
  hasFrom() const
  {
    return m_hasFields[REPO_PARAMETER_FROM];
  }

  const Block&
  getTo() const
  {
    return m_to;
  }

  RepoCommandParameter&
  setTo(const Block& to);

  bool
  hasTo() const
  {
    return m_hasFields[REPO_PARAMETER_TO];
  }


  uint64_t
  getStartBlockId() const
  {
    assert(hasStartBlockId());
    return m_startBlockId;
  }

  RepoCommandParameter&
  setStartBlockId(uint64_t startBlockId);

  bool
  hasStartBlockId() const
  {
    return m_hasFields[REPO_PARAMETER_START_BLOCK_ID];
  }

  uint64_t
  getEndBlockId() const
  {
    assert(hasEndBlockId());
    return m_endBlockId;
  }

  RepoCommandParameter&
  setEndBlockId(uint64_t endBlockId);

  bool
  hasEndBlockId() const
  {
    return m_hasFields[REPO_PARAMETER_END_BLOCK_ID];
  }

  uint64_t
  getProcessId() const
  {
    assert(hasProcessId());
    return m_processId;
  }

  RepoCommandParameter&
  setProcessId(uint64_t processId);

  bool
  hasProcessId() const
  {
    return m_hasFields[REPO_PARAMETER_PROCESS_ID];
  }

  milliseconds
  getInterestLifetime() const
  {
    assert(hasInterestLifetime());
    return m_interestLifetime;
  }

  RepoCommandParameter&
  setInterestLifetime(milliseconds interestLifetime);

  bool
  hasInterestLifetime() const
  {
    return m_hasFields[REPO_PARAMETER_INTEREST_LIFETIME];
  }

  const Block&
  getClusterPrefix() const
  {
    assert(hasClusterPrefix());
    return m_clusterPrefix;
  }

  RepoCommandParameter&
  setClusterPrefix(const Block& clusterPrefix);

  bool
  hasClusterPrefix() const
  {
    return m_hasFields[REPO_PARAMETER_CLUSTER_PREFIX];
  }

  const std::vector<bool>&
  getPresentFields() const {
    return m_hasFields;
  }

  template<ndn::encoding::Tag T>
  size_t
  wireEncode(EncodingImpl<T>& block) const;

  Block
  wireEncode() const;

  void
  wireDecode(const Block& wire);

private:
  std::vector<bool> m_hasFields;
  ndn::DelegationList m_nodePrefix;
  Name m_name;
  Block m_from, m_to;
  uint64_t m_startBlockId;
  uint64_t m_endBlockId;
  uint64_t m_processId;
  milliseconds m_interestLifetime;
  Block m_clusterPrefix;

  mutable Block m_wire;
};

NDN_CXX_DECLARE_WIRE_ENCODE_INSTANTIATIONS(RepoCommandParameter);

} // namespace repo

#endif // REPO_REPO_COMMAND_PARAMETER_HPP
