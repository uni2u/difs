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

#include "read-handle.hpp"
#include "repo.hpp"
#include "../manifest/manifest.hpp"
#include "util.hpp"

#include <ndn-cxx/util/logger.hpp>
#include <ndn-cxx/util/random.hpp>

namespace repo {

NDN_LOG_INIT(repo.ReadHandle);

static const milliseconds DEFAULT_INTEREST_LIFETIME(4000);

ReadHandle::ReadHandle(Face &face, KeySpaceHandle& keySpaceHandle, RepoStorage &storageHandle,
                       Scheduler &scheduler, Validator &validator,
                       size_t prefixSubsetLength,
                       ndn::Name const &clusterNodePrefix)
  : CommandBaseHandle(face, storageHandle, scheduler, validator)
  , m_prefixSubsetLength(prefixSubsetLength)
  , m_face(face)
  , m_storageHandle(storageHandle)
  , m_interestLifetime(DEFAULT_INTEREST_LIFETIME)
  , m_clusterNodePrefix(clusterNodePrefix)
  , m_keySpaceHandle(keySpaceHandle)
{
  connectAutoListen();
}

void
ReadHandle::connectAutoListen()
{
  // Connect a RepoStorage's signals to the read handle
  if (m_prefixSubsetLength != RepoConfig::DISABLED_SUBSET_LENGTH) {
    afterDataInsertionConnection = m_storageHandle.afterDataInsertion.connect(
      [this] (const Name& prefix) {
        onDataInserted(prefix);
      });
    afterDataDeletionConnection = m_storageHandle.afterDataDeletion.connect(
      [this] (const Name& prefix) {
        onDataDeleted(prefix);
      });
  }
}

void
ReadHandle::onInterest(const Name& prefix, const Interest& interest)
{
  NDN_LOG_DEBUG("Received Interest " << interest.getName());
  std::shared_ptr<ndn::Data> data = m_storageHandle.readData(interest);
  if (data != nullptr) {
    NDN_LOG_DEBUG("Put Data: " << *data);
    m_face.put(*data);
  }
  else {
    NDN_LOG_DEBUG("No data for " << interest.getName());
  }
}

void
ReadHandle::onGetInterest(const Name& prefix, const Interest& interest)
{
  RepoCommandParameter parameter;
  try {
    extractParameter(interest, prefix, parameter);
  }
  catch (RepoCommandParameter::Error&) {
    negativeReply(interest, "Parameter malformed", 403);
    return;
  }
  auto name = parameter.getName();
  NDN_LOG_DEBUG("Received get interest " << name);
  auto hash = Manifest::getHash(name.toUri());
  auto repo = m_keySpaceHandle.getManifestStorage(hash);

  NDN_LOG_DEBUG("Find " << hash << " from " << repo);
  ProcessId processId = ndn::random::generateWord64();
  ProcessInfo& process = m_processes[processId];
  process.interest = interest;

  RepoCommandParameter parameters;
  parameters.setName(hash);
  parameters.setProcessId(processId);

  Interest findInterest = util::generateCommandInterest(
    repo, "find", parameters, m_interestLifetime);
  findInterest.setMustBeFresh(true);

  m_face.expressInterest(
    findInterest,
    std::bind(&ReadHandle::onFindCommandResponse, this, _1, _2, processId),
    std::bind(&ReadHandle::onFindCommandTimeout, this, _1, processId),
    std::bind(&ReadHandle::onFindCommandTimeout, this, _1, processId));
}

void
ReadHandle::onFindCommandResponse(const Interest& interest, const Data& data, ProcessId processId)
{
  auto process = m_processes[processId];
  Data responseData(process.interest.getName());
  auto content = data.getContent();
  std::string json(
    content.value_begin(),
    content.value_end());

  if (json.length() == 0) {
    NDN_LOG_DEBUG("Manifest not found");
    reply(process.interest, "");
  } else {
    NDN_LOG_DEBUG("Forward manifest " << json);
    reply(process.interest, json);
  }
  m_processes.erase(processId);
}

void
ReadHandle::onFindCommandTimeout(const Interest& interest, ProcessId processId)
{
  NDN_LOG_DEBUG("Find command timeout");
  negativeReply(m_processes[processId].interest, "Manifest timeout", 403);
  m_processes.erase(processId);
}

void
ReadHandle::onRegisterFailed(const Name& prefix, const std::string& reason)
{
  NDN_LOG_ERROR("ERROR: Failed to register prefix in local hub's daemon");
  m_face.shutdown();
}

void
ReadHandle::listen(const Name& prefix)
{
  ndn::InterestFilter filter(Name(prefix).append("data"));
  m_face.setInterestFilter(filter,
                           std::bind(&ReadHandle::onInterest, this, _1, _2),
                           std::bind(&ReadHandle::onRegisterFailed, this, _1, _2));

  ndn::InterestFilter filterGet(Name("get"));
  m_face.setInterestFilter(filterGet,
                           std::bind(&ReadHandle::onGetInterest, this, _1, _2),
                           std::bind(&ReadHandle::onRegisterFailed, this, _1, _2));

  NDN_LOG_DEBUG("read handle listen complete");
}

void
ReadHandle::onDataDeleted(const Name& name)
{
  // We add one here to account for the implicit digest at the end,
  // which is what we get from the underlying storage when deleting.
  Name prefix = name.getPrefix(-(m_prefixSubsetLength + 1));
  auto check = m_insertedDataPrefixes.find(prefix);
  if (check != m_insertedDataPrefixes.end()) {
    if (--(check->second.useCount) <= 0) {
      check->second.hdl.unregister();
      m_insertedDataPrefixes.erase(prefix);
    }
  }
}

void
ReadHandle::onDataInserted(const Name& name)
{
  // Note: We want to save the prefix that we register exactly, not the
  // name that provoked the registration
  Name prefixToRegister = name.getPrefix(-m_prefixSubsetLength);
  ndn::InterestFilter filter("/");
  auto check = m_insertedDataPrefixes.find(prefixToRegister);
  if (check == m_insertedDataPrefixes.end()) {
    // Because of stack lifetime problems, we assume here that the
    // prefix registration will be successful, and we add the registered
    // prefix to our list. This is because, if we fail, we shut
    // everything down, anyway. If registration failures are ever
    // considered to be recoverable, we would need to make this
    // atomic.
    auto hdl = m_face.setInterestFilter(filter,
      [this] (const ndn::InterestFilter& filter, const Interest& interest) {
        // Implicit conversion to Name of filter
        onInterest(filter, interest);
      },
      [] (const Name&) {},
      [this] (const Name& prefix, const std::string& reason) {
        onRegisterFailed(prefix, reason);
      });
    RegisteredDataPrefix registeredPrefix{hdl, 1};
    // Newly registered prefix
    m_insertedDataPrefixes.emplace(std::make_pair(prefixToRegister, registeredPrefix));
  }
  else {
    check->second.useCount++;
  }
}

} // namespace repo
