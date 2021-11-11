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

#include "repo.hpp"
#include "storage/fs-storage.hpp"
#include "storage/mongodb-storage.hpp"
#include "repo-command-parameter.hpp"

#include <ndn-cxx/util/logger.hpp>
#include <ndn-cxx/security/command-interest-signer.hpp>
#include <ndn-cxx/security/pib/identity-container.hpp>
#include "ndn-cxx/security/certificate.hpp"

#include "ndn-cxx/security/security-common.hpp"
#include "ndn-cxx/security/pib/certificate-container.hpp"

namespace repo {

NDN_LOG_INIT(repo.Repo);

RepoConfig
parseConfig(const std::string& configPath)
{
  if (configPath.empty()) {
    NDN_LOG_DEBUG("configuration file path is empty");
  }

  std::ifstream fin(configPath.c_str());
  if (!fin.is_open())
    BOOST_THROW_EXCEPTION(Repo::Error("failed to open configuration file '" + configPath + "'"));

  using namespace boost::property_tree;
  ptree propertyTree;
  try {
    read_info(fin, propertyTree);
  }
  catch (const ptree_error& e) {
    BOOST_THROW_EXCEPTION(Repo::Error("failed to read configuration file '" + configPath + "'"));
  }

  ptree repoConf = propertyTree.get_child("repo");

  RepoConfig repoConfig;
  repoConfig.repoConfigPath = configPath;

  ptree dataConf = repoConf.get_child("data");
  for (const auto& section : dataConf) {
    if (section.first == "prefix")
      repoConfig.dataPrefixes.push_back(Name(section.second.get_value<std::string>()));
    else if (section.first == "registration-subset")
      repoConfig.registrationSubset = section.second.get_value<int>();
    else
      BOOST_THROW_EXCEPTION(Repo::Error("Unrecognized '" + section.first + "' option in 'data' section in "
                                        "configuration file '"+ configPath +"'"));
  }

  // ptree commandConf = repoConf.get_child("command");
  // for (const auto& section : commandConf) {
  //   if (section.first == "prefix")
  //     repoConfig.repoPrefixes.push_back(Name(section.second.get_value<std::string>()));
  //   else
  //     BOOST_THROW_EXCEPTION(Repo::Error("Unrecognized '" + section.first + "' option in 'command' section in "
  //                                       "configuration file '"+ configPath +"'"));
  // }

  auto tcpBulkInsert = repoConf.get_child_optional("tcp_bulk_insert");
  bool isTcpBulkEnabled = false;
  std::string host = "localhost";
  std::string port = "7376";
  if (tcpBulkInsert) {
    for (const auto& section : *tcpBulkInsert) {
      isTcpBulkEnabled = true;
      if (section.first == "host") {
        host = section.second.get_value<std::string>();
      }
      else if (section.first == "port") {
        port = section.second.get_value<std::string>();
      }
      else
        BOOST_THROW_EXCEPTION(Repo::Error("Unrecognized '" + section.first + "' option in 'tcp_bulk_insert' section in "
                                          "configuration file '"+ configPath +"'"));
    }
  }
  if (isTcpBulkEnabled) {
    repoConfig.tcpBulkInsertEndpoints.push_back(std::make_pair(host, port));
  }

  ptree storageConf = repoConf.get_child("storage");
  std::string storageMethod = storageConf.get<std::string>("method");
  if (storageMethod == "fs") {
    repoConfig.storageMethod = StorageMethod::STORAGE_METHOD_SQLITE;
  }
  else if (storageMethod == "mongodb"){
    repoConfig.storageMethod = StorageMethod::STORAGE_METHOD_MONGODB;
    repoConfig.mongodb.db = storageConf.get<std::string>("mongodb.db");
  }
  else {
    BOOST_THROW_EXCEPTION(Repo::Error("Only 'fs' or 'mongodb' storage method is supported"));
  }

  repoConfig.fs.dbPath = repoConf.get<std::string>("storage.fs.path");

  repoConfig.validatorNode = repoConf.get_child("validator");

  repoConfig.nMaxPackets = repoConf.get<uint64_t>("storage.max-packets");

  repoConfig.clusterNodePrefix= Name(repoConf.get<std::string>("cluster.nodePrefix"));
  repoConfig.clusterPrefix = repoConf.get<std::string>("cluster.prefix");
  repoConfig.clusterType = repoConf.get<std::string>("cluster.type");
  if (repoConfig.clusterType == "node") {
    repoConfig.managerPrefix = Name(repoConf.get<std::string>("cluster.managerPrefix"));
    repoConfig.from = repoConf.get<std::string>("cluster.from");
    repoConfig.to = repoConf.get<std::string>("cluster.to");
  }

  return repoConfig;
}

std::shared_ptr<Storage>
createStorage(const RepoConfig& config)
{
  if (config.storageMethod == StorageMethod::STORAGE_METHOD_MONGODB) {
    return std::make_shared<MongoDBStorage>(config.mongodb.db);
  }
  else {
  // else if (config.storageMethod == StorageMethod::STORAGE_METHOD_SQLITE) {
    return std::make_shared<FsStorage>(config.fs.dbPath);
  }
}

Repo::Repo(boost::asio::io_service& ioService, std::shared_ptr<Storage> storage, const RepoConfig& config)
  : m_config(config)
  , m_scheduler(ioService)
  , m_face(ioService)
  , m_dispatcher(m_face, m_hcKeyChain)
  , m_store(storage)
  , m_storageHandle(*m_store)
  , m_validator(m_face)  
  , m_keySpaceHandle(m_face, m_storageHandle, m_dispatcher, m_scheduler, m_validator, m_config.clusterNodePrefix, m_config.clusterPrefix, m_config.managerPrefix, m_config.clusterType, m_config.from)
  , m_readHandle(m_face, m_keySpaceHandle, m_storageHandle, m_scheduler, m_validator, m_config.registrationSubset, m_config.clusterNodePrefix)
  , m_writeHandle(m_face, m_keySpaceHandle, m_storageHandle, m_dispatcher, m_scheduler, m_validator, m_config.clusterNodePrefix, m_config.clusterPrefix)
  , m_infoHandle(m_face, m_storageHandle, m_scheduler, m_validator, m_config.clusterNodePrefix, m_config.clusterPrefix)
  , m_deleteHandle(m_face, m_keySpaceHandle, m_storageHandle, m_dispatcher, m_scheduler, m_validator, m_config.clusterNodePrefix, m_config.clusterPrefix)
  , m_manifestHandle(m_face, m_storageHandle, m_dispatcher, m_scheduler, m_validator, m_config.clusterNodePrefix, m_config.clusterPrefix)
  , m_tcpBulkInsertHandle(ioService, m_storageHandle)
{
  this->enableValidation();

  auto& ids = m_hcKeyChain.getPib().getIdentities();

  for(auto identity: ids) {
    auto keyPrefix = identity.getName();
    keyPrefix.append(ndn::name::Component("KEY"));
    std::cout<<"Register Key prefix identity:"<<keyPrefix<<std::endl;
    m_face.setInterestFilter(
        ndn::InterestFilter(keyPrefix),
        bind(&Repo::onKeyInterest, this, _1, _2),
        bind(&Repo::onKeyRegisterFailed, this, _1, _2)); 
  }
}

void
Repo::addNode() {
  if (m_config.clusterType != "node") 
    return;

  RepoCommandParameter parameter;
  Block to = ndn::encoding::makeBinaryBlock(tlv::To, m_config.to.c_str(), m_config.to.length());
  Block from = ndn::encoding::makeBinaryBlock(tlv::From, m_config.from.c_str(), m_config.from.length());
  parameter.setTo(to);
  parameter.setFrom(from);
  Name cmd = m_config.managerPrefix;
  cmd
    .append("add-node")
    .append(parameter.wireEncode());

  ndn::HCKeyChain hcKeyChain;
  ndn::security::CommandInterestSigner cmdSigner(hcKeyChain);

  Interest addInterest = cmdSigner.makeCommandInterest(cmd);
  addInterest.setInterestLifetime(3_s);
  addInterest.setCanBePrefix(true);
  addInterest.setMustBeFresh(true);

  m_face.expressInterest(
    addInterest,
    std::bind(&Repo::onAddCommandResponse, this, _1, _2),
    std::bind(&Repo::onAddCommandTimeout, this, _1),
    std::bind(&Repo::onAddCommandTimeout, this, _1));  
}

void
Repo::onKeyInterest(const ndn::InterestFilter&, const Interest&interest)
{
  std::cout << "Got interest for certificate. Interest: " << std::endl;
    try{
      Name identity = ndn::security::v2::extractIdentityFromKeyName(interest.getName());
      // const auto cert = getCertificateFromPib(m_hcKeyChain.getPib(), identity, true, false, false);
      const auto cert = m_hcKeyChain.getPib().getDefaultIdentity().getDefaultKey().getCertificate(identity);
      // const auto cert = m_hcKeyChain.getPib().getIdentity(identity).getKey(identity).getCertificate(identity);
      m_face.put(cert);
      } catch(std::exception& e) {
        std::cout << "Certificate is not found for: " << interest << std::endl;
        }
}

void 
Repo::onKeyRegisterSuccess(const ndn::Name& name)
{
  std::cout<<"Register Key successful!" <<name.toUri()<<std::endl;
  m_face.shutdown();
}

void 
Repo::onKeyRegisterFailed(const ndn::Name& prefix, const std::string& reason)
{
  std::cout<<"Register failed for prefix " <<reason.c_str()<<std::endl;
  m_face.shutdown();
}

void
Repo::onAddCommandResponse(const Interest& interest, const Data& data) {
  std::cout << "Node add success" << std::endl;
}

void
Repo::onAddCommandTimeout(const Interest& interest) {
  std::cout << "Node add timeout" << std::endl;
}

void
Repo::initializeStorage()
{
  // Rebuild storage if storage checkpoin exists
  ndn::time::steady_clock::TimePoint start = ndn::time::steady_clock::now();
  ndn::time::steady_clock::TimePoint end = ndn::time::steady_clock::now();
  ndn::time::milliseconds cost = ndn::time::duration_cast<ndn::time::milliseconds>(end - start);
  NDN_LOG_DEBUG("initialize storage cost: " << cost << "ms");
}

void
Repo::enableListening()
{
  auto clusterPrefix = Name(m_config.clusterPrefix);
  auto selfNodePrefix = Name(m_config.clusterNodePrefix).append(m_config.clusterPrefix);

  m_face.registerPrefix(clusterPrefix, nullptr,
    [] (const Name& clusterPrefix, const std::string& reason) {
      NDN_LOG_DEBUG("Cluster prefix: " << clusterPrefix << " registration error: " << reason);
    }
  );

  m_face.registerPrefix(selfNodePrefix, nullptr,
    [] (const Name& selfNodePrefix, const std::string& reason) {
      NDN_LOG_DEBUG("Self Node prefix: " << selfNodePrefix << " registration error: " << reason);
    }
  );

  m_readHandle.listen(selfNodePrefix);

  m_dispatcher.addTopPrefix(clusterPrefix);

  for (const auto& ep : m_config.tcpBulkInsertEndpoints) {
    m_tcpBulkInsertHandle.listen(ep.first, ep.second);
  }
}

void
Repo::enableValidation()
{
  m_validator.load(m_config.validatorNode, m_config.repoConfigPath);
}

} // namespace repo
