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

#ifndef REPO_REPO_HPP
#define REPO_REPO_HPP

#include "common.hpp"
#include "handles/delete-handle.hpp"
#include "handles/manifest-handle.hpp"
#include "handles/read-handle.hpp"
#include "handles/tcp-bulk-insert-handle.hpp"
#include "handles/write-handle.hpp"
#include "handles/info-handle.hpp"
#include "handles/keyspace-handle.hpp"
#include "storage/repo-storage.hpp"
#include "storage/storage-method.hpp"

#include <memory>

#include <ndn-cxx/security/hc-key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <ndn-cxx/mgmt/dispatcher.hpp>
#include <ndn-cxx/security/validator-config.hpp>
#include <ndn-cxx/security/key-chain.hpp>

namespace repo {

struct Fs
{
  std::string dbPath;
};

struct MongoDB
{
  std::string db;
};

struct RepoConfig
{
  static const size_t DISABLED_SUBSET_LENGTH = -1;

  std::string repoConfigPath;
  StorageMethod storageMethod;
  Fs fs;
  MongoDB mongodb;
  std::vector<ndn::Name> dataPrefixes;
  size_t registrationSubset = DISABLED_SUBSET_LENGTH;
  std::vector<ndn::Name> repoPrefixes;
  std::vector<std::pair<std::string, std::string>> tcpBulkInsertEndpoints;
  uint64_t nMaxPackets;
  boost::property_tree::ptree validatorNode;

  //DIFS
  ndn::Name clusterNodePrefix;
  std::string clusterPrefix;
  std::string clusterType;
  ndn::Name managerPrefix;
  std::string from, to;
};

RepoConfig
parseConfig(const std::string& confPath);

std::shared_ptr<Storage>
createStorage(const RepoConfig& config);

class Repo : noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

public:
  Repo(boost::asio::io_service& ioService, std::shared_ptr<Storage> storage, const RepoConfig& config);

  //@brief rebuild index from storage file when repo starts.
  void
  initializeStorage();

  void
  enableListening();

  void
  enableValidation();

  void
  onAddCommandResponse(const Interest& interest, const Data& data);

  void
  onAddCommandTimeout(const Interest& interest);

  void
  addNode();

  void
  onKeyInterest(const ndn::InterestFilter&, const Interest&interest);

  void 
  onKeyRegisterSuccess(const ndn::Name& name);

  void 
  onKeyRegisterFailed(const ndn::Name& prefix, const std::string& reason);

private:
  RepoConfig m_config;
  Scheduler m_scheduler;
  Face m_face;
  ndn::mgmt::Dispatcher m_dispatcher;
  std::shared_ptr<Storage> m_store;
  RepoStorage m_storageHandle;
  HCKeyChain m_hcKeyChain;
  // ndn::KeyChain m_keyChain;
  ValidatorConfig m_validator;

  KeySpaceHandle m_keySpaceHandle;
  ReadHandle m_readHandle;
  WriteHandle m_writeHandle;
  InfoHandle m_infoHandle;
  DeleteHandle m_deleteHandle;
  ManifestHandle m_manifestHandle;

  TcpBulkInsertHandle m_tcpBulkInsertHandle;
  std::string m_keySpaceFile;
};

} // namespace repo

#endif // REPO_REPO_HPP
