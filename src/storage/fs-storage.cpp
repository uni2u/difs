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

#include "fs-storage.hpp"
#include "config.hpp"

#include <ndn-cxx/util/sha256.hpp>
#include <ndn-cxx/util/sqlite3-statement.hpp>

#include <boost/filesystem.hpp>
#include <boost/compute/detail/sha1.hpp>
#include <istream>
#include <memory>

#include <ndn-cxx/util/logger.hpp>

namespace repo {

NDN_LOG_INIT(repo.FsStorage);

const char* FsStorage::DIRNAME_DATA = "data";
const char* FsStorage::DIRNAME_MANIFEST = "manifest";

int64_t
FsStorage::hash(std::string const& key)
{
  uint64_t result = 12345;
  for (auto current = key.begin(); current < key.end(); current += 1) {
    result = 127 * result + static_cast<unsigned char>(*current);
  }
  return result;
}

std::string
FsStorage::sha1Hash(std::string const& key)
{
  boost::compute::detail::sha1 sha1;
  sha1.process(key);
  return std::string(sha1);
}

boost::filesystem::path
FsStorage::getPath(const Name& name, const char* dataType)
{
  auto hash = sha1Hash(name.toUri());
  auto dir1 = hash.substr(0, 2);
  auto dir2 = hash.substr(2, hash.size() - 2);
  return m_path / dataType / dir1 / dir2;
}

FsStorage::FsStorage(const std::string& dbPath)
{
  if (dbPath.empty()) {
    std::cerr << "Create db path in local location [" << dbPath << "]. " << std::endl;
    m_dbPath = "ndn_repo";
  }
  else {
    boost::filesystem::path fsPath(dbPath);
    boost::filesystem::file_status fsPathStatus = boost::filesystem::status(fsPath);
    if (!boost::filesystem::is_directory(fsPathStatus)) {
      if (!boost::filesystem::create_directories(boost::filesystem::path(fsPath))) {
        BOOST_THROW_EXCEPTION(Error("Directory '" + dbPath + "' does not exists and cannot be created"));
      }
    }

    m_dbPath = dbPath;
  }
  m_path = boost::filesystem::path(m_dbPath);
  boost::filesystem::create_directory(m_path / DIRNAME_DATA);
  boost::filesystem::create_directory(m_path / DIRNAME_MANIFEST);
}

FsStorage::~FsStorage()
{
}

int64_t
FsStorage::writeData(const Data& data, const char* dataType)
{
  auto name = data.getName();
  std::cout << "Saving... " << name.toUri() << std::endl;
  auto id = hash(name.toUri());

  boost::filesystem::path fsPath = getPath(data.getName(), dataType);
  boost::filesystem::create_directories(fsPath.parent_path());

  std::ofstream outFileData(fsPath.string(), std::ios::binary);
  outFileData.write(
      reinterpret_cast<const char*>(data.wireEncode().wire()),
      data.wireEncode().size());

  return (int64_t)id;
}

int64_t
FsStorage::insert(const Data& data)
{
  return writeData(data, DIRNAME_DATA);
}

std::string
FsStorage::insertManifest(const Manifest& manifest)
{
  boost::filesystem::path fsPath = m_path / DIRNAME_MANIFEST / manifest.getHash();

  auto json = manifest.toJson();

  std::ofstream outFile(fsPath.string());
  outFile.write(
      json.c_str(),
      json.size());

  return manifest.getHash();
}

bool
FsStorage::erase(const Name& name)
{
  auto fsPath = getPath(name, DIRNAME_DATA);

  boost::filesystem::file_status fsPathStatus = boost::filesystem::status(fsPath);
  if (!boost::filesystem::exists(fsPathStatus)) {
    NDN_LOG_DEBUG(name.toUri() << " is not exists (" << fsPath << ")");
    return false;
  }

  boost::filesystem::remove_all(fsPath);
  return true;
}

bool
FsStorage::eraseManifest(const std::string& hash)
{
  boost::filesystem::path fsPath = m_path / DIRNAME_MANIFEST / hash;

  boost::filesystem::file_status fsPathStatus = boost::filesystem::status(fsPath);
  if (!boost::filesystem::exists(fsPathStatus)) {
    std::cerr << hash << " is not exists (" << fsPath << ")" << std::endl;
    return false;
  }

  boost::filesystem::remove_all(fsPath);
  return true;
}

std::shared_ptr<Data>
FsStorage::read(const Name& name)
{
  auto fsPath = getPath(name.toUri(), DIRNAME_DATA);

  boost::filesystem::ifstream inFileData(fsPath, std::ifstream::binary);
  if (!inFileData.is_open()) {
    return nullptr;
  }
  auto data = std::make_shared<Data>();

  inFileData.seekg(0, inFileData.end);
  int length = inFileData.tellg();
  inFileData.seekg(0, inFileData.beg);

  char * buffer = new char [length];
  inFileData.read(buffer, length);

  data->wireDecode(Block(reinterpret_cast<const uint8_t*>(buffer), length));

  return data;
}

std::shared_ptr<Manifest>
FsStorage::readManifest(const std::string& hash)
{
  boost::filesystem::path fsPath = m_path / DIRNAME_MANIFEST / hash;
  boost::filesystem::ifstream inFileData(fsPath);

  //FIXME: check exists
  if (!inFileData.is_open()) {
    return nullptr;
  }

  std::string json(
      (std::istreambuf_iterator<char>(inFileData)),
      std::istreambuf_iterator<char>());

  return std::make_shared<Manifest>(Manifest::fromJson(json));
}

bool
FsStorage::has(const Name& name)
{
  auto fsPath = getPath(name, DIRNAME_DATA);
  auto fsPathStatus = boost::filesystem::status(fsPath);
  return boost::filesystem::exists(fsPathStatus);
}

bool
FsStorage::hasManifest(const std::string& hash)
{
  auto fsPath = m_path / DIRNAME_MANIFEST / hash;
  auto fsPathStatus = boost::filesystem::status(fsPath);
  return boost::filesystem::exists(fsPathStatus);
}

uint64_t
FsStorage::size()
{
  int64_t size = 0;

  for (auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(m_path / DIRNAME_DATA), {})) {
    std::ignore = entry;
    size += 1;
  }

  return size;
}

} // namespace repo
