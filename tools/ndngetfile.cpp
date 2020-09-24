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

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/delegation-list.hpp>

#include <fstream>
#include <iostream>
#include <unistd.h>

#include <boost/lexical_cast.hpp>

#include "../src/manifest/manifest.hpp"
#include "../src/util.hpp"

namespace repo {

using ndn::Name;
using ndn::Interest;
using ndn::Data;

class Consumer : boost::noncopyable
{
public:
  Consumer(const std::string& dataName, std::ostream& os,
           bool verbose, bool versioned, bool single,
           int interestLifetime, int timeout,
           bool mustBeFresh = false,
           bool canBePrefix = false)
    : m_dataName(dataName)
    , m_os(os)
    , m_verbose(verbose)
    , m_isFinished(false)
    , m_isFirst(true)
    , m_interestLifetime(interestLifetime)
    , m_timeout(timeout)
    , m_currentSegment(0)
    , m_totalSize(0)
    , m_retryCount(0)
    , m_mustBeFresh(mustBeFresh)
    , m_canBePrefix(canBePrefix)
  {
  }

  void
  run();

private:
  void
  fetchData(const Manifest& manifest, uint64_t segmentId);

  ndn::Name
  selectRepoName(const Manifest& manifest, uint64_t segmentId);

  void
  onManifest(const Interest& interest, const ndn::Data& data);

  void
  onManifestTimeout(const ndn::Interest& interest);

  void
  onUnversionedData(const ndn::Interest& interest, const ndn::Data& data);

  void
  onTimeout(const ndn::Interest& interest);

  void
  readData(const ndn::Data& data);

  void
  fetchNextData();

private:
  ndn::Face m_face;
  ndn::Name m_dataName;
  std::ostream& m_os;
  bool m_verbose;
  bool m_isFinished;
  bool m_isFirst;
  ndn::time::milliseconds m_interestLifetime;
  ndn::time::milliseconds m_timeout;
  uint64_t m_currentSegment;
  int m_totalSize;
  int m_retryCount;
  bool m_mustBeFresh;
  bool m_canBePrefix;

  std::shared_ptr<Manifest> m_manifest;
  uint64_t m_finalBlockId;

  static constexpr int MAX_RETRY = 3;
};

void
Consumer::fetchData(const Manifest& manifest, uint64_t segmentId)
{
  auto repoName = selectRepoName(manifest, segmentId);
  auto name = manifest.getName();
  auto clusterName = repoName.getSubName(0, repoName.size() - 1);
  Interest interest(clusterName.append("data").append(name).appendSegment(segmentId));
  interest.setInterestLifetime(m_interestLifetime);
  // interest.setMustBeFresh(true);

  ndn::Delegation d;
  d.name = ndn::Name(repoName);
  // ndn::DelegationList dl = { d };
  interest.setForwardingHint(ndn::DelegationList{d});

  m_face.expressInterest(interest,
                         std::bind(&Consumer::onUnversionedData, this, _1, _2),
                         std::bind(&Consumer::onTimeout, this, _1), // Nack
                         std::bind(&Consumer::onTimeout, this, _1));
}

ndn::Name
Consumer::selectRepoName(const Manifest& manifest, uint64_t segmentId)
{
  auto repos = manifest.getRepos();
  for (auto iter = repos.begin(); iter != repos.end(); ++iter)
  {
    auto start = iter->start;
    auto end = iter->end;
    if (start <= (int)segmentId && (int)segmentId <= end)
    {
      return Name(iter->name);
    }
  }

  // Should not be here
  return Name("");
}

void
Consumer::run()
{
  // Get manifest
  RepoCommandParameter parameter;
  parameter.setName(m_dataName);
  Interest interest = util::generateCommandInterest(Name("get"), "", parameter, m_interestLifetime);
  
  std::cerr << interest << std::endl;

  m_face.expressInterest(
    interest,
    std::bind(&Consumer::onManifest, this, _1, _2),
    std::bind(&Consumer::onManifestTimeout, this, _1),
    std::bind(&Consumer::onManifestTimeout, this, _1));

  // processEvents will block until the requested data received or timeout occurs
  m_face.processEvents(m_timeout);
}

void
Consumer::onManifest(const Interest& interest, const Data& data)
{
  auto content = data.getContent();
  std::string json(
    content.value_begin(),
    content.value_end()
  );

  if (json.length() == 0) {
    std::cerr << "Not found" << std::endl;
    return;
  }

  auto manifest = Manifest::fromJson(json);
  m_manifest = std::make_shared<Manifest>(manifest);

  m_finalBlockId = manifest.getEndBlockId();
  std::cerr << "final block: " << m_finalBlockId;

  fetchData(manifest, m_currentSegment);
}

void
Consumer::onManifestTimeout(const Interest& interest)
{
  if (m_retryCount++ < MAX_RETRY) {
    // Retransmit the interest
    RepoCommandParameter parameter;
    parameter.setName(m_dataName);
    Interest interest = util::generateCommandInterest(Name("get"), "", parameter, m_interestLifetime);
    
    std::cerr << interest << std::endl;

    m_face.expressInterest(
      interest,
      std::bind(&Consumer::onManifest, this, _1, _2),
      std::bind(&Consumer::onManifestTimeout, this, _1),
      std::bind(&Consumer::onManifestTimeout, this, _1));
    if (m_verbose) {
      std::cerr << "TIMEOUT: retransmit interest for manifest"<< std::endl;
    }
  }
  else {
    std::cerr << "TIMEOUT: last interest sent for manifest" << std::endl;
    std::cerr << "TIMEOUT: abort fetching after " << MAX_RETRY
              << " times of retry" << std::endl;
  }
}

void
Consumer::onUnversionedData(const Interest& interest, const Data& data)
{
  fetchNextData();
  readData(data);
}

void
Consumer::readData(const Data& data)
{
  const auto& content = data.getContent();
  m_os.write(reinterpret_cast<const char*>(content.value()), content.value_size());
  m_totalSize += content.value_size();
  if (m_verbose) {
    std::cerr << "LOG: received data = " << data.getName() << std::endl;
  }
  if (m_isFinished) {
    std::cerr << "INFO: End of file is reached" << std::endl;
    std::cerr << "INFO: Total # of segments received: " << m_currentSegment + 1  << std::endl;
    std::cerr << "INFO: Total # bytes of content received: " << m_totalSize << std::endl;
  }
}

void
Consumer::fetchNextData()
{
  if (m_currentSegment >= m_finalBlockId) {
    m_isFinished = true;
  } else {
    m_retryCount = 0;
    m_currentSegment += 1;
    fetchData(*m_manifest, m_currentSegment);
  }
}

void
Consumer::onTimeout(const Interest& interest)
{
  if (m_retryCount++ < MAX_RETRY) {
    // Retransmit the interest
    fetchData(*m_manifest, m_currentSegment);
    if (m_verbose) {
      std::cerr << "TIMEOUT: retransmit interest for " << interest.getName() << std::endl;
    }
  }
  else {
    std::cerr << "TIMEOUT: last interest sent for segment #" << m_currentSegment << std::endl;
    std::cerr << "TIMEOUT: abort fetching after " << MAX_RETRY
              << " times of retry" << std::endl;
  }
}

static void
usage(const char* programName)
{
  std::cerr << "Usage: "
            << programName << " [-v] [-l lifetime] [-w timeout] [-o filename] ndn-name\n"
            << "\n"
            << "  -v: be verbose\n"
            << "  -l: InterestLifetime in milliseconds\n"
            << "  -w: timeout in milliseconds for whole process (default unlimited)\n"
            << "  -o: write to local file name instead of stdout\n"
            << "  ndn-name: NDN Name prefix for Data to be read\n"
            << std::endl;
}

static int
main(int argc, char** argv)
{
  std::string name;
  const char* outputFile = nullptr;
  bool verbose = false, versioned = false, single = false;
  int interestLifetime = 4000;  // in milliseconds
  int timeout = 0;  // in milliseconds

  int opt;
  while ((opt = getopt(argc, argv, "hvl:w:o:")) != -1) {
    switch (opt) {
    case 'h':
      usage(argv[0]);
      return 0;
    case 'v':
      verbose = true;
      break;
    case 'l':
      try {
        interestLifetime = boost::lexical_cast<int>(optarg);
      }
      catch (const boost::bad_lexical_cast&) {
        std::cerr << "ERROR: -l option should be an integer" << std::endl;
        return 2;
      }
      interestLifetime = std::max(interestLifetime, 0);
      break;
    case 'w':
      try {
        timeout = boost::lexical_cast<int>(optarg);
      }
      catch (const boost::bad_lexical_cast&) {
        std::cerr << "ERROR: -w option should be an integer" << std::endl;
        return 2;
      }
      timeout = std::max(timeout, 0);
      break;
    case 'o':
      outputFile = optarg;
      break;
    default:
      usage(argv[0]);
      return 2;
    }
  }

  if (optind < argc) {
    name = argv[optind];
  }

  if (name.empty()) {
    usage(argv[0]);
    return 2;
  }

  std::streambuf* buf;
  std::ofstream of;

  if (outputFile != nullptr) {
    of.open(outputFile, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!of || !of.is_open()) {
      std::cerr << "ERROR: cannot open " << outputFile << std::endl;
      return 2;
    }
    buf = of.rdbuf();
  }
  else {
    buf = std::cout.rdbuf();
  }

  std::ostream os(buf);
  Consumer consumer(name, os, verbose, versioned, single, interestLifetime, timeout);

  // try {
    consumer.run();
  // }
  // catch (const std::exception& e) {
  //   std::cerr << "ERROR: " << e.what() << std::endl;
  //   return 1;
  // }

  return 0;
}

} // namespace repo

int
main(int argc, char** argv)
{
  return repo::main(argc, argv);
}
