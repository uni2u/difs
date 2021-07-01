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

#include "info-handle.hpp"

#include <boost/property_tree/json_parser.hpp>

#include <ndn-cxx/util/logger.hpp>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>

namespace repo {

NDN_LOG_INIT(repo.InfoHandle);

static const int DEFAULT_CREDIT = 12;
static const bool DEFAULT_CANBE_PREFIX = false;
static const milliseconds MAX_TIMEOUT(60_s);
static const milliseconds NOEND_TIMEOUT(10000_ms);
static const milliseconds PROCESS_DELETE_TIME(10000_ms);
static const milliseconds DEFAULT_INTEREST_LIFETIME(4000_ms);

InfoHandle::InfoHandle(Face& face, RepoStorage& storageHandle,
                        Scheduler& scheduler, Validator& validator,
                        ndn::Name const& clusterNodePrefix, std::string clusterPrefix)
  : CommandBaseHandle(face, storageHandle, scheduler, validator)
  , m_repoPrefix(Name(clusterNodePrefix).append(clusterPrefix))
{
  ndn::InterestFilter filterInfo = Name(m_repoPrefix).append("info");
  face.setInterestFilter(filterInfo,
                           std::bind(&InfoHandle::handleInfoCommand, this, _1, _2),
                           std::bind(&InfoHandle::onRegisterFailed, this, _1, _2));
}

void
InfoHandle::handleInfoCommand(const Name& prefix, const Interest& interest)
{
 namespace pt = boost::property_tree;
 pt::ptree root, disk, memory, diskNode, memoryNode;

 struct statvfs sv;
 statvfs("/",&sv);

 diskNode.put("size", ((long long)sv.f_blocks * sv.f_bsize / 1024));
 diskNode.put("usage", ((long long)sv.f_bavail * sv.f_bsize / 1024));
 disk.add_child("disk", diskNode);

 struct sysinfo si;
 sysinfo(&si);

 memoryNode.put("size", ((long long)si.totalram / 1024));
 memoryNode.put("usage", ((long long)si.freeram / 1024)) ;
 memory.add_child("memory", memoryNode);

 auto datas = CommandBaseHandle::storageHandle.readDatas();
 auto manifests = CommandBaseHandle::storageHandle.readManifests();

 root.put("name", prefix.toUri());
 root.add_child("disk", disk);
 root.add_child("memory", memory);
 root.add_child("datas", datas);
 root.add_child("manifests", manifests);

 std::stringstream os;
 pt::write_json(os, root, false);

 reply(interest, os.str());
}

void
InfoHandle::onRegisterFailed(const Name& prefix, const std::string& reason)
{
  NDN_LOG_ERROR("ERROR: Failed to register prefix in local hub's daemon");
  face.shutdown();
}

}
