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

#include "mongodb-storage.hpp"
#include "config.hpp"

#include <istream>

#include <boost/compute/detail/sha1.hpp>
#include <boost/filesystem.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <ndn-cxx/util/logger.hpp>
#include <ndn-cxx/util/sha256.hpp>
#include <ndn-cxx/util/sqlite3-statement.hpp>


namespace repo {

using std::string;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;

NDN_LOG_INIT(repo.MongoDBStorage);

// const char* MongoDBStorage::DIRNAME_DATA = "data";
// const char* MongoDBStorage::DIRNAME_MANIFEST = "manifest";

int64_t
MongoDBStorage::hash(string const& key)
{
  uint64_t result = 12345;
  for (auto current = key.begin(); current < key.end(); current += 1) {
    result = 127 * result + static_cast<unsigned char>(*current);
  }
  return result;
}

MongoDBStorage::MongoDBStorage(const string& dbName, const string& collectionName)
  : mInstance(mongocxx::instance{})
  , mClient(mongocxx::client{mongocxx::uri{}})
{
  mDB = mClient[dbName];
  mCollection = mDB[collectionName];
}

MongoDBStorage::~MongoDBStorage()
{
}

int64_t
MongoDBStorage::insert(const Data& data)
{
  auto key = data.getName().at(-2).toUri();
  bsoncxx::document::view_or_value filter = document{}
	  << "key" << key
    << finalize;
  bsoncxx::types::b_binary dataBinary;
  dataBinary.bytes = data.wireEncode().wire();
  dataBinary.size = data.wireEncode().size();

  bsoncxx::document::view_or_value replacement = document{}
	  << "key" << key
    << "value" << dataBinary
	  << finalize;

  mongocxx::options::replace options;
  options.upsert(true);
  mCollection.replace_one(filter, replacement, options);
  
  auto id = hash(data.getName().toUri());
  return id;
}

string
MongoDBStorage::insertManifest(const Manifest& manifest)
{
  bsoncxx::document::view_or_value filter = document{}
    << "manifest_hash" << manifest.getHash()
    << finalize;

  auto json = manifest.toJson();
  bsoncxx::document::view_or_value replacement = document{}
    << "manifest_hash" << manifest.getHash()
    << "manifest_json" << json
    << finalize;

  mongocxx::options::replace options;
  options.upsert(true);
  mCollection.replace_one(filter, replacement, options);

  return manifest.getHash();
}

// TODO: Not implemented
bool
MongoDBStorage::erase(const Name& name)
{
  return true;
}

// TODO: Not implemented
bool
MongoDBStorage::eraseManifest(const string& hash)
{
  return true;
}

std::shared_ptr<Data>
MongoDBStorage::read(const Name& name)
{
  auto maybe_result = mCollection.find_one(document{}
    << "key" << name.at(-2).toUri()
    << finalize);

  if (!maybe_result) {
    return nullptr;
  }

  bsoncxx::types::b_binary dataBinary = maybe_result.value().view()["value"].get_binary();
  auto data = std::make_shared<Data>();
  data->wireDecode(Block(dataBinary.bytes, dataBinary.size));
  return data;
}

std::shared_ptr<Manifest>
MongoDBStorage::readManifest(const string& hash)
{
  auto maybe_result = mCollection.find_one(document{}
    << "manifest_hash" << hash
    << finalize);

  if (!maybe_result) {
    NDN_LOG_DEBUG("Manifest doen't exists");
    return nullptr;
  }

  string json = maybe_result.value().view()["manifest_json"].get_utf8().value.to_string();
  return std::make_shared<Manifest>(Manifest::fromJson(json));
}

// TODO: Not implemented
bool
MongoDBStorage::has(const Name& name)
{
  return false;
}

// TODO: Not implemented
bool
MongoDBStorage::hasManifest(const string& hash)
{
  return false;
}

// TODO: Not implemented
uint64_t
MongoDBStorage::size()
{
  return 0;
}

} // namespace repo
