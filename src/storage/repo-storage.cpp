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

#include "repo-storage.hpp"
#include "config.hpp"

#include <istream>

#include <ndn-cxx/util/exception.hpp>
#include <ndn-cxx/util/logger.hpp>

namespace repo {

NDN_LOG_INIT(repo.RepoStorage);

RepoStorage::RepoStorage(Storage& store)
  : m_storage(store)
{
}

bool
RepoStorage::insertData(const Data& data)
{
  bool isExist = m_storage.has(data.getFullName());

  if (isExist) {
    NDN_LOG_DEBUG("Data already in storage, regarded as successful data insertion");
    return true;
  }

  int64_t id = m_storage.insert(data);
  if (id == NOTFOUND)
    return false;

  afterDataInsertion(data.getName());
  return true;
}

ssize_t
RepoStorage::deleteData(const Name& name)
{
  NDN_LOG_DEBUG("Delete: " << name);

  if (m_storage.erase(name)) {
    return 1;
  }
  return -1;
}

ssize_t
RepoStorage::deleteData(const Interest& interest)
{
  return deleteData(interest.getName());
}

std::shared_ptr<Data>
RepoStorage::readData(const Interest& interest) const
{
  NDN_LOG_DEBUG("Reading data for " << interest.getName());

  return m_storage.read(interest.getName());
}

bool
RepoStorage::insertManifest(const Manifest& manifest)
{
  NDN_LOG_DEBUG("Insert manifest for " << manifest.getHash());

  m_storage.insertManifest(manifest);

  return true;
}

std::shared_ptr<Manifest>
RepoStorage::readManifest(const std::string& hash)
{
  NDN_LOG_DEBUG("Reading manifest for " << hash);

  return m_storage.readManifest(hash);
}

bool
RepoStorage::deleteManifest(const std::string& hash)
{
  if (m_storage.eraseManifest(hash)) {
    return 1;
  }

  return -1;
}

std::shared_ptr<boost::property_tree::ptree>
RepoStorage::readDatas()
{
  NDN_LOG_DEBUG("Reading datas");

  return m_storage.readDatas();
}

std::shared_ptr<boost::property_tree::ptree>
RepoStorage::readManifests()
{
  NDN_LOG_DEBUG("Reading manifests");

  return m_storage.readManifests();
}


} // namespace repo
