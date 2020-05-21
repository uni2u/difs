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

#ifndef REPO_STORAGE_FS_STORAGE_HPP
#define REPO_STORAGE_FS_STORAGE_HPP

#include "storage.hpp"

#include <algorithm>
#include <iostream>
#include <queue>
#include <stdlib.h>
#include <string>
#include <sqlite3.h>
#include <vector>

namespace repo {

class FsStorage : public Storage
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

  explicit
  FsStorage(const std::string& dbPath);

  ~FsStorage();

  /**
   *  @brief  put the data into database
   *  @param  data     the data should be inserted into databse
   *  @return int64_t  the id number of each entry in the database
   */
  int64_t
  insert(const Data& data) override;

  std::string
  insertManifest(const Manifest& data) override;

  /**
   *  @brief  remove the entry in the database by using name as index
   *  @param  name   name of the data
   */
  bool
  erase(const Name& name) override;

  bool
  eraseManifest(const std::string& hash) override;

  std::shared_ptr<Data>
  read(const Name& name) override;

  Manifest
  readManifest(const std::string& hash) override;

  bool
  has(const Name& name) override;

  bool
  hasManifest(const std::string& hash) override;

  /**
   *  @brief  return the size of database
   */
  uint64_t
  size() override;

private:
  int64_t
  hash(std::string const& key);

  std::string
  sha1Hash(std::string const& key);

  boost::filesystem::path
  getPath(const Name& name, const char* dataType);

  int64_t
  writeData(const Data& data, const char* dataType);

private:
  std::string m_dbPath;
  boost::filesystem::path m_path;

  static const char* FNAME_NAME;
  static const char* FNAME_DATA;
  static const char* FNAME_HASH;
  static const char* DIRNAME_DATA;
  static const char* DIRNAME_MANIFEST;
};


} // namespace repo

#endif // REPO_STORAGE_FS_STORAGE_HPP
