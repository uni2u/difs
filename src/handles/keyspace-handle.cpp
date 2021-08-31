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

#include "keyspace-handle.hpp"
#include "util.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <boost/lexical_cast.hpp>

#include <ndn-cxx/security/command-interest-signer.hpp>
#include <ndn-cxx/security/hc-key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/util/logger.hpp>

namespace pt = boost::property_tree;
namespace repo {

NDN_LOG_INIT(repo.KeySpaceHandle);

static const int DEFAULT_CREDIT = 12;
static const bool DEFAULT_CANBE_PREFIX = false;
static const milliseconds MAX_TIMEOUT(60_s);
static const milliseconds NOEND_TIMEOUT(10000_ms);
static const milliseconds PROCESS_DELETE_TIME(10000_ms);
static const milliseconds DEFAULT_INTEREST_LIFETIME(4000_ms);

void
KeySpaceHandle::initKeySpaceFile() {
  pt::ptree root, keySpaces, keySpaceNode;
  keySpaceNode.put("node", m_repoPrefix.toUri());
  keySpaceNode.put("start", "0x00");
  keySpaceNode.put("end", "0xff");

  keySpaces.push_back(std::make_pair("", keySpaceNode));

  root.add_child("keyspaces", keySpaces);

  std::stringstream os;
  pt::write_json(os, root, false);
  m_keySpaceFile = os.str();

  m_version = "v" + std::to_string(m_versionNum++);
}

KeySpaceHandle::KeySpaceHandle(Face& face, RepoStorage& storageHandle, ndn::mgmt::Dispatcher& dispatcher,
                         Scheduler& scheduler, Validator& validator,
                         ndn::Name const& clusterNodePrefix, std::string clusterPrefix, ndn::Name const& managerPrefix, 
                         std::string clusterType, std::string from)
  : CommandBaseHandle(face, storageHandle, scheduler, validator)
  , m_versionNum(0)
  , m_credit(DEFAULT_CREDIT)
  , m_canBePrefix(DEFAULT_CANBE_PREFIX)
  , m_maxTimeout(MAX_TIMEOUT)
  , m_noEndTimeout(NOEND_TIMEOUT)
  , m_interestLifetime(DEFAULT_INTEREST_LIFETIME)
  , m_managerPrefix(managerPrefix)
  , m_clusterType(clusterType)
  , m_from(from)
  , m_repoPrefix(Name(clusterNodePrefix).append(clusterPrefix))
{
  if (m_clusterType == "manager")
    initKeySpaceFile();

  ndn::InterestFilter filterAddNode = Name(m_repoPrefix).append("add-node");
  face.setInterestFilter(filterAddNode,
                           std::bind(&KeySpaceHandle::handleAddCommand, this, _1, _2),
                           std::bind(&KeySpaceHandle::onRegisterFailed, this, _1, _2));

  ndn::InterestFilter filterDelNode = Name(m_repoPrefix).append("del-node");
  face.setInterestFilter(filterDelNode,
                           std::bind(&KeySpaceHandle::handleDeleteCommand, this, _1, _2),
                           std::bind(&KeySpaceHandle::onRegisterFailed, this, _1, _2));

  ndn::InterestFilter filterVer = Name(m_repoPrefix).append("keyspace").append("ver");
  face.setInterestFilter(filterVer,
                           std::bind(&KeySpaceHandle::handleVersionCommand, this, _1, _2),
                           std::bind(&KeySpaceHandle::onRegisterFailed, this, _1, _2));

  ndn::InterestFilter filterFetch= Name(m_repoPrefix).append("keyspace").append("fetch");
  face.setInterestFilter(filterFetch,
                           std::bind(&KeySpaceHandle::handleFetchCommand, this, _1, _2),
                           std::bind(&KeySpaceHandle::onRegisterFailed, this, _1, _2));

  ndn::InterestFilter filterCoordination = Name(m_repoPrefix).append("coordination");
  face.setInterestFilter(filterCoordination,
                           std::bind(&KeySpaceHandle::handleCoordinationCommand, this, _1, _2),
                           std::bind(&KeySpaceHandle::onRegisterFailed, this, _1, _2));

  ndn::InterestFilter filterManifestList = Name(m_repoPrefix).append("manifestlist");
  face.setInterestFilter(filterManifestList,
                           std::bind(&KeySpaceHandle::handleManifestListCommand, this, _1, _2),
                           std::bind(&KeySpaceHandle::onRegisterFailed, this, _1, _2));

  ndn::InterestFilter filterComplete = Name(m_repoPrefix).append("complete");
  face.setInterestFilter(filterComplete,
                           std::bind(&KeySpaceHandle::handleCompleteCommand, this, _1, _2),
                           std::bind(&KeySpaceHandle::onRegisterFailed, this, _1, _2));

  ndn::InterestFilter filterRingInfo = Name(clusterPrefix).append("ringinfo");
  face.setInterestFilter(filterRingInfo,
                           std::bind(&KeySpaceHandle::handleRingInfoCommand, this, _1, _2),
                           std::bind(&KeySpaceHandle::onRegisterFailed, this, _1, _2));
}

void
KeySpaceHandle::handleVersionCommand(const Name& prefix, const Interest& interest)
{
  auto version= interest.getName().at(-1).toUri();
  if(version != m_version)
    onFetchCommand(version);

  negativeReply(interest, "", 200);
}

void
KeySpaceHandle::onVersionCommand() 
{
  pt::ptree root, keySpaces;
  std::istringstream keyFile(m_keySpaceFile);

  pt::read_json(keyFile, root);
  keySpaces = root.get_child("keyspaces");

  for(auto it = keySpaces.begin(); it != keySpaces.end(); it++) {
    if(it == keySpaces.begin())
      continue;

    auto nodeName = it->second.get<std::string>("node");
    
    Name cmd = Name(nodeName);
    cmd
      .append("keyspace")
      .append("ver")
      .append(m_version);

    Interest verInterest(cmd);
    verInterest.setCanBePrefix(true);
    verInterest.setMustBeFresh(false);
    verInterest.setInterestLifetime(6_s);

    face.expressInterest(
      verInterest,
      std::bind(&KeySpaceHandle::onVersionCommandResponse, this, _1, _2),
      std::bind(&KeySpaceHandle::onVersionCommandTimeout, this, _1),
      std::bind(&KeySpaceHandle::onVersionCommandTimeout, this, _1));
  }
}

void
KeySpaceHandle::onVersionCommandResponse(const Interest& interest, const Data& data) 
{
  NDN_LOG_DEBUG("Version Command Response");
}

void
KeySpaceHandle::onVersionCommandTimeout(const Interest& interest) 
{
  onVersionCommand();
  NDN_LOG_ERROR("Version Command Timeout");
}

void
KeySpaceHandle::handleFetchCommand(const Name& prefix, const Interest& interest)
{
  auto version= interest.getName().at(-1).toUri();
  if (version == m_version) {
    reply(interest, m_keySpaceFile);
  } else {
    reply(interest, "");
  }

  onCoordinationCommand();
}

void
KeySpaceHandle::onFetchCommand(std::string versionNum) {
  Name cmd = m_managerPrefix; 
  cmd
    .append("keyspace")
    .append("fetch")
    .append(versionNum);

  Interest fetchInterest(cmd);
  fetchInterest.setCanBePrefix(true);
  fetchInterest.setMustBeFresh(false);
  fetchInterest.setInterestLifetime(6_s);

  face.expressInterest(
    fetchInterest,
    std::bind(&KeySpaceHandle::onFetchCommandResponse, this, _1, _2),
    std::bind(&KeySpaceHandle::onFetchCommandTimeout, this, _1),
    std::bind(&KeySpaceHandle::onFetchCommandTimeout, this, _1));
}

void
KeySpaceHandle::onFetchCommandResponse(const Interest& interest, const Data& data)
{
  auto content = data.getContent();
  if(content.value_size() == 0) {
    NDN_LOG_ERROR("Keyspacefile Version Diff");
    return;
  }

  m_version = interest.getName().at(-1).toUri();
  m_keySpaceFile = reinterpret_cast<const char*>(content.value());
  m_keySpaceFile = m_keySpaceFile.substr(0, content.value_size());

  pt::ptree root, keySpaces;
  std::istringstream keyFile(m_keySpaceFile);
  pt::read_json(keyFile, root);
  keySpaces = root.get_child("keyspaces");

  for(auto it = keySpaces.begin(); it != keySpaces.end(); it++) {
    auto node = it->second;
    auto nodeName = node.get<std::string>("node");

    if(nodeName == m_repoPrefix.toUri()) {
      m_start = stoi(node.get<std::string>("start"), 0, 16);
      m_end = stoi(node.get<std::string>("end"), 0, 16);

      break;
    }
  }
}

void
KeySpaceHandle::onFetchCommandTimeout(const Interest& interest)
{
  onFetchCommand(interest.getName().at(-1).toUri());
  NDN_LOG_ERROR("Fetch timeout");
}

void
KeySpaceHandle::handleAddCommand(const Name& prefix, const Interest& interest)
{
  RepoCommandParameter repoParameter;
  extractParameter(interest, prefix, repoParameter);

  m_from = reinterpret_cast<const char*>(repoParameter.getFrom().value());
  m_from = m_from.substr(0, repoParameter.getFrom().value_size());

  m_to = reinterpret_cast<const char*>(repoParameter.getTo().value());
  m_to = m_to.substr(0, repoParameter.getTo().value_size());

  pt::ptree root, keySpaces, toNode;
  std::istringstream keyFile(m_keySpaceFile);

  pt::read_json(keyFile, root);
  keySpaces = root.get_child("keyspaces");
  root.pop_front();

  for(auto it = keySpaces.begin(); it != keySpaces.end(); it++) {
    auto nodeName = it->second.get<std::string>("node");

    if(nodeName == m_from) {
      auto start = stoi(it->second.get<std::string>("start"), 0, 16);
      auto end = stoi(it->second.get<std::string>("end"), 0, 16);
      auto hashSize = (end - start) / 2;

      auto fromStart = end - hashSize;
      auto fromEnd = end;
      end = fromStart - 1;

      std::stringstream stream;
      stream << "0x" << std::hex << end;
      it->second.put("end", stream.str());
      stream.str(""); 

      // add to node
      toNode.put("node", m_to);
      stream << "0x" << std::hex << fromStart;
      toNode.put("start", stream.str()); 

      stream.str("");
      stream << "0x" << std::hex << fromEnd;
      toNode.put("end", stream.str());

      keySpaces.push_back(std::make_pair("", toNode));
      root.put_child("keyspaces", keySpaces);

      std::stringstream os;
      pt::write_json(os, root, false);

      m_keySpaceFile = os.str();
      m_version = "v" + std::to_string(m_versionNum++);

      negativeReply(interest, "", 200);
      onVersionCommand();

      break;
    }
  }
}

void
KeySpaceHandle::handleDeleteCommand(const Name& prefix, const Interest& interest)
{
  RepoCommandParameter repoParameter;
  extractParameter(interest, prefix, repoParameter);

  m_from = reinterpret_cast<const char*>(repoParameter.getFrom().value());
  m_from = m_from.substr(0, repoParameter.getFrom().value_size());

  m_to = reinterpret_cast<const char*>(repoParameter.getTo().value());
  m_to = m_to.substr(0, repoParameter.getTo().value_size());

  namespace pt = boost::property_tree;
  pt::ptree root, keySpaces, keySpaceNode, fromNode, toNode;
  int fromStart = 0, fromEnd = 0, toStart;
  std::istringstream keyFile(m_keySpaceFile);
  std::stringstream stream;

  pt::read_json(keyFile, root);
  keySpaces = root.get_child("keyspaces");
  root.pop_front();

  for (auto it = keySpaces.begin(); it != keySpaces.end(); it++) {
    auto node = it->second;
    auto nodeName = node.get<std::string>("node");

    if (m_from == nodeName) {
      fromStart = stoi(node.get<std::string>("start"), 0, 16);
      fromEnd = stoi(node.get<std::string>("end"), 0, 16);
      keySpaces.erase(it);
      break;
    }
  }

  for (auto it = keySpaces.begin(); it != keySpaces.end(); it++) {
    auto nodeName = it->second.get<std::string>("node");

    if (m_to == nodeName) {
      toStart = stoi(it->second.get<std::string>("start"), 0, 16);

      if(fromEnd < toStart) {
        stream << "0x" << std::hex << fromStart;
        it->second.put("start", stream.str());
      } else {
        stream << "0x" << std::hex << fromEnd;
        it->second.put("end", stream.str());
      }
    }
  }

  root.put_child("keyspaces", keySpaces);

  std::stringstream os;
  pt::write_json(os, root, false);

  m_keySpaceFile = os.str();
  m_version = "v" + std::to_string(m_versionNum++);

  negativeReply(interest, "", 200);
  onVersionCommand();
}

void
KeySpaceHandle::handleRingInfoCommand(const Name& prefix, const Interest& interest) 
{
  reply(interest, m_keySpaceFile);
}

void
KeySpaceHandle::handleManifestListCommand(const Name& prefix, const Interest& interest) 
{
  pt::ptree root;
  auto manifests = CommandBaseHandle::storageHandle.readManifests();

  root.add_child("manifests", manifests);

  std::stringstream os;
  pt::write_json(os, root, false);

  reply(interest, os.str());
}

void
KeySpaceHandle::onManifestListCommand() 
{
  Name cmd = Name(m_from);
  cmd
    .append("manifestlist");

  Interest manifestListInterest(cmd);
  manifestListInterest.setCanBePrefix(true);
  manifestListInterest.setMustBeFresh(false);
  manifestListInterest.setInterestLifetime(6_s);

  face.expressInterest(
    manifestListInterest,
    std::bind(&KeySpaceHandle::onManifestListCommandResponse, this, _1, _2),
    std::bind(&KeySpaceHandle::onManifestListCommandTimeout, this, _1),
    std::bind(&KeySpaceHandle::onManifestListCommandTimeout, this, _1));
}

void
KeySpaceHandle::onManifestListCommandResponse(const Interest& interest, const Data& data)
{
  auto content = data.getContent();
  if(content.value_size() == 0) {
    NDN_LOG_ERROR("ManifestList Command Failed");
    return;
  }

  m_manifestList = reinterpret_cast<const char*>(content.value());
  m_manifestList = m_manifestList.substr(0, content.value_size());

  pt::ptree root, manifests;
  std::istringstream manifestList(m_manifestList);

  pt::read_json(manifestList, root);
  manifests = root.get_child("manifests");

  for (auto it = manifests.begin(); it != manifests.end(); it++) {
    auto node = it->second;
    auto manifestName = node.get<std::string>("key");

    for (auto start = m_start; start <= m_end; start++) {
      std::stringstream stream;
      stream << std::hex << start;

      if (!strncmp(manifestName.c_str(), stream.str().c_str(), stream.str().length())) {
        onManifestCommand(manifestName);
      }
    }
  }

  onCompleteCommand();
}

void
KeySpaceHandle::onManifestListCommandTimeout(const Interest& interest)
{
  onManifestListCommand();
  NDN_LOG_ERROR("Manifest List timeout");
}

void
KeySpaceHandle::handleManifestCommand(const Name& prefix, const Interest& interest) 
{
  auto manifestName = interest.getName().at(-1).toUri();
  reply(interest, "");
}

void
KeySpaceHandle::onManifestCommand(std::string manifestName) 
{
  RepoCommandParameter parameter;
  parameter.setName(manifestName);

  Interest manifestInterest = util::generateCommandInterest(
   m_from, "find", parameter, 4_s);
  manifestInterest.setCanBePrefix(true);
  manifestInterest.setMustBeFresh(false);

  face.expressInterest(
    manifestInterest,
    std::bind(&KeySpaceHandle::onManifestCommandResponse, this, _1, _2),
    std::bind(&KeySpaceHandle::onManifestCommandTimeout, this, _1),
    std::bind(&KeySpaceHandle::onManifestCommandTimeout, this, _1));
}

void
KeySpaceHandle::onManifestCommandResponse(const Interest& interest, const Data& data)
{
  auto content = data.getContent();
  std::string json(
    content.value_begin(),
    content.value_end());

  auto manifest = Manifest::fromJson(json);
  CommandBaseHandle::storageHandle.insertManifest(manifest);
}

void
KeySpaceHandle::onManifestCommandTimeout(const Interest& interest)
{
  // onManifestListCommand(interest.getName().at(-2).toUri());
  NDN_LOG_ERROR("Manifest timeout");
}

void
KeySpaceHandle::onRegisterFailed(const Name& prefix, const std::string& reason) 
{
  NDN_LOG_ERROR("ERROR: Failed to register prefix in local hub's daemon");
  face.shutdown();
}

// =======================Update==============================
void
KeySpaceHandle::handleCoordinationCommand(const Name& prefix, const Interest& interest)
{
  RepoCommandParameter repoParameter;
  extractParameter(interest, prefix, repoParameter);

  if (repoParameter.hasFrom()) {
    m_from = reinterpret_cast<const char *>(repoParameter.getFrom().value());
    m_from = m_from.substr(0, repoParameter.getFrom().value_size());
  }

  negativeReply(interest, "", 200);

  onManifestListCommand();
}

void
KeySpaceHandle::onCoordinationCommand() 
{
  RepoCommandParameter parameter;
  parameter.setFrom(ndn::encoding::makeBinaryBlock(tlv::From, m_from.c_str(), m_from.length()));
  Name cmd = Name(m_to);
  cmd
    .append("coordination")
    .append(parameter.wireEncode());

  ndn::HCKeyChain hcKeyChain;
  ndn::security::CommandInterestSigner cmdSigner(hcKeyChain);

  Interest coordinationInterest = cmdSigner.makeCommandInterest(cmd);
  coordinationInterest.setInterestLifetime(3_s);
  coordinationInterest.setCanBePrefix(true);
  coordinationInterest.setMustBeFresh(false);

  face.expressInterest(
    coordinationInterest,
    std::bind(&KeySpaceHandle::onCoordinationCommandResponse, this, _1, _2),
    std::bind(&KeySpaceHandle::onCoordinationCommandTimeout, this, _1),
    std::bind(&KeySpaceHandle::onCoordinationCommandTimeout, this, _1));
}

void
KeySpaceHandle::onCoordinationCommandResponse(const Interest& interest, const Data& data)
{
  NDN_LOG_DEBUG("Coordination Command Response");
}

void
KeySpaceHandle::onCoordinationCommandTimeout(const Interest& interest)
{
  onCoordinationCommand();
  NDN_LOG_ERROR("Coordination timeout");
}

void
KeySpaceHandle::handleCompleteCommand(const Name& prefix, const Interest& interest)
{
  negativeReply(interest, "", 200);
}

void
KeySpaceHandle::onCompleteCommand() 
{
  Name cmd = Name(m_managerPrefix);
  cmd
    .append("complete");

  Interest completeInterest(cmd);
  completeInterest.setCanBePrefix(true);
  completeInterest.setMustBeFresh(false);
  completeInterest.setInterestLifetime(6_s);

  face.expressInterest(
    completeInterest,
    std::bind(&KeySpaceHandle::onCompleteCommandResponse, this, _1, _2),
    std::bind(&KeySpaceHandle::onCompleteCommandTimeout, this, _1),
    std::bind(&KeySpaceHandle::onCompleteCommandTimeout, this, _1));
}

void
KeySpaceHandle::onCompleteCommandResponse(const Interest& interest, const Data& data)
{
  pt::ptree root, manifests;
  std::istringstream manifestList(m_manifestList);

  pt::read_json(manifestList, root);
  manifests = root.get_child("manifests");

  for (auto it = manifests.begin(); it != manifests.end(); it++) {
    auto manifestName = it->second.get<std::string>("key");

    for (auto start = m_start; start <= m_end; start++) {
      std::stringstream stream;
      stream << std::hex << start;

      if (!strncmp(manifestName.c_str(), stream.str().c_str(), stream.str().length())) {
        onDeleteManifestCommand(manifestName);
      }
    } 
  }
}

void
KeySpaceHandle::onCompleteCommandTimeout(const Interest& interest)
{
  NDN_LOG_ERROR("Complete timeout");
}

void
KeySpaceHandle::onDeleteManifestCommand(std::string manifestName) 
{
  RepoCommandParameter parameter;
  parameter.setName(manifestName);

  Interest delManifestInterest = util::generateCommandInterest(
   m_from, "only-delete-manifest", parameter, 4_s);
  delManifestInterest.setCanBePrefix(true);
  delManifestInterest.setMustBeFresh(false);

  face.expressInterest(
    delManifestInterest,
    std::bind(&KeySpaceHandle::onDeleteManifestCommandResponse, this, _1),
    std::bind(&KeySpaceHandle::onDeleteManifestCommandTimeout, this, _1),
    std::bind(&KeySpaceHandle::onDeleteManifestCommandTimeout, this, _1));
}

void
KeySpaceHandle::onDeleteManifestCommandResponse(const Interest& interest)
{
  NDN_LOG_DEBUG("Delete Manifest Command Response");
}

void
KeySpaceHandle::onDeleteManifestCommandTimeout(const Interest& interest)
{
  NDN_LOG_ERROR("Delete Manifest Command Tiemout");
}

ndn::Name
KeySpaceHandle::getManifestStorage(const std::string hash)
{
  pt::ptree root, keySpaces;
  std::istringstream keySpaceFile(m_keySpaceFile);

  pt::read_json(keySpaceFile, root);
  keySpaces = root.get_child("keyspaces");

  for (auto it = keySpaces.begin(); it != keySpaces.end(); it++) {
    auto node = it->second;
    auto nodeName = node.get<std::string>("node");

    auto start = stoi(node.get<std::string>("start"), 0, 16);
    auto end = stoi(node.get<std::string>("end"), 0, 16);

    for (; start <= end; start++) {
      std::stringstream stream;
      stream << std::hex << start; 
      
      std::string startHash = str(boost::format("%02x") % stream.str());

      if(!strncmp(hash.c_str(), startHash.c_str(),startHash.length())) {
        return nodeName;
      }
    }
  }

  return Name("");
}

}