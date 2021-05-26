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

#include <boost/compute/detail/sha1.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <ndn-cxx/util/logger.hpp>

namespace repo {

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;

NDN_LOG_INIT(repo.MongoDBStorage);

const char* MongoDBStorage::COLLNAME_DATA = "data";
const char* MongoDBStorage::COLLNAME_MANIFEST = "manifest";
const string MongoDBStorage::FIELDNAME_KEY = "key";
const string MongoDBStorage::FIELDNAME_VALUE = "value";

int64_t
MongoDBStorage::hash(string const& key)
{
  uint64_t result = 12345;
  for (auto current = key.begin(); current < key.end(); current += 1) {
    result = 127 * result + static_cast<unsigned char>(*current);
  }
  return result;
}

string
MongoDBStorage::sha1Hash(std::string const& key)
{
  boost::compute::detail::sha1 sha1;
  sha1.process(key);
  return std::string(sha1);
}

MongoDBStorage::MongoDBStorage(const string& dbName)
  : mInstance(mongocxx::instance{})
  , mClient(mongocxx::client{mongocxx::uri{}})
{
  mDB = mClient[dbName];
}

MongoDBStorage::~MongoDBStorage()
{
}

int64_t
MongoDBStorage::insert(const Data& data)
{
  mongocxx::collection coll = mDB[COLLNAME_DATA];
  string key = sha1Hash(data.getName().toUri());

  bsoncxx::document::view_or_value filter = document{}
    << FIELDNAME_KEY << key
    << finalize;

  bsoncxx::types::b_binary dataBinary;
  dataBinary.bytes = data.wireEncode().wire();
  dataBinary.size = data.wireEncode().size();

  bsoncxx::document::view_or_value replacement = document{}
    << FIELDNAME_KEY << key
    << FIELDNAME_VALUE << dataBinary
    << finalize;

  mongocxx::options::replace options;
  options.upsert(true);
  coll.replace_one(filter, replacement, options);
  
  auto id = hash(data.getName().toUri());
  return id;
}

string
MongoDBStorage::insertManifest(const Manifest& manifest)
{
  mongocxx::collection coll = mDB[COLLNAME_MANIFEST];

  bsoncxx::document::view_or_value filter = document{}
    << FIELDNAME_KEY << manifest.getHash()
    << finalize;

  auto json = manifest.toJson();
  bsoncxx::document::view_or_value replacement = document{}
    << FIELDNAME_KEY << manifest.getHash()
    << FIELDNAME_VALUE << json
    << finalize;

  mongocxx::options::replace options;
  options.upsert(true);
  coll.replace_one(filter, replacement, options);

  return manifest.getHash();
}

bool
MongoDBStorage::erase(const Name& name)
{
  mongocxx::collection coll = mDB[COLLNAME_DATA];
  string key = sha1Hash(name.toUri());

  auto maybe_result = coll.find_one(document{}
    << FIELDNAME_KEY << key
    << finalize);

  if (!maybe_result) {
    NDN_LOG_DEBUG(name.toUri() << " is not exists (" << key << ")");
    return false;
  }

  coll.delete_one(document{} << FIELDNAME_KEY << key << finalize);
  return true;
}

bool
MongoDBStorage::eraseManifest(const string& hash)
{
  mongocxx::collection coll = mDB[COLLNAME_MANIFEST];

  auto maybe_result = coll.find_one(document{}
    << FIELDNAME_KEY << hash
    << finalize);
  if (!maybe_result) {
    string info = "DB: " + mDB.name().to_string() +
               ", Coll: " + coll.name().to_string() +
               ", " + FIELDNAME_KEY + ": " + hash;
    std::cerr << hash << " is not exists (" << info << ")" << std::endl;
    return false;
  }

  coll.delete_one(document{} << FIELDNAME_KEY << hash << finalize);
  return true;
}

std::shared_ptr<Data>
MongoDBStorage::read(const Name& name)
{
  std::cout << "size(): " << size() << std::endl;
  mongocxx::collection coll = mDB[COLLNAME_DATA];
  string key = sha1Hash(name.toUri());

  auto maybe_result = coll.find_one(document{}
    << FIELDNAME_KEY << key
    << finalize);

  if (!maybe_result) {
    return nullptr;
  }

  bsoncxx::types::b_binary dataBinary = maybe_result.value().view()[FIELDNAME_VALUE].get_binary();
  auto data = std::make_shared<Data>();
  data->wireDecode(Block(dataBinary.bytes, dataBinary.size));
  return data;
}

std::shared_ptr<Manifest>
MongoDBStorage::readManifest(const string& hash)
{
  mongocxx::collection coll = mDB[COLLNAME_MANIFEST];

  auto maybe_result = coll.find_one(document{}
    << FIELDNAME_KEY << hash
    << finalize);

  if (!maybe_result) {
    NDN_LOG_DEBUG("Manifest doen't exists");
    return nullptr;
  }

  string json = maybe_result.value().view()[FIELDNAME_VALUE].get_utf8().value.to_string();
  return std::make_shared<Manifest>(Manifest::fromJson(json));
}

std::shared_ptr<boost::property_tree::ptree>
MongoDBStorage::readDatas()
{
  namespace pt = boost::property_tree;
  std::shared_ptr<pt::ptree> root = std::make_shared<pt::ptree>();

  mongocxx::collection coll = mDB[COLLNAME_DATA];
  auto cursor = coll.find({});

  for (auto doc: cursor) {
    bsoncxx::types::b_binary dataBinary = doc[FIELDNAME_VALUE].get_binary();
    auto data = std::make_shared<Data>();
    data->wireDecode(Block(dataBinary.bytes, dataBinary.size));

    std::shared_ptr<pt::ptree> node = std::make_shared<pt::ptree>();
    node->put("data", data->getName().toUri());
    root->push_back(std::make_pair("", *node));
  }

  return root;
}

std::shared_ptr<boost::property_tree::ptree>
MongoDBStorage::readManifests()
{
  namespace pt = boost::property_tree;
  std::shared_ptr<pt::ptree> root = std::make_shared<pt::ptree>();

  mongocxx::collection coll = mDB[COLLNAME_MANIFEST];
  auto cursor = coll.find({});

  for (auto doc: cursor) {
    std::string json = doc[FIELDNAME_VALUE].get_utf8().value.to_string();
    auto manifest = std::make_shared<Manifest>(Manifest::fromJson(json));

    std::shared_ptr<pt::ptree> node = std::make_shared<pt::ptree>();
    node->put("key", manifest->getName());
    root->push_back(std::make_pair("", *node));
  }

  return root;
}

bool
MongoDBStorage::has(const Name& name)
{
  mongocxx::collection coll = mDB[COLLNAME_DATA];
  string key = sha1Hash(name.toUri());

  auto maybe_result = coll.find_one(document{}
    << FIELDNAME_KEY << key
    << finalize);

  if (maybe_result) {
    return true;
  }
  else {
    return false;
  }
}

bool
MongoDBStorage::hasManifest(const string& hash)
{
  mongocxx::collection coll = mDB[COLLNAME_MANIFEST];

  auto maybe_result = coll.find_one(document{}
    << FIELDNAME_KEY << hash
    << finalize);

  if (maybe_result) {
    return true;
  }
  else {
    return false;
  }
}

uint64_t
MongoDBStorage::size()
{
  mongocxx::collection coll = mDB[COLLNAME_DATA];

  return coll.count_documents(document{} << finalize);
}

} // namespace repo
