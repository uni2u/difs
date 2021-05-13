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

#include "../src/repo-command-parameter.hpp"
#include "../src/repo-command-response.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/command-interest-signer.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <string>
#include <list>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/read.hpp>
#include <boost/filesystem.hpp>
#include <boost/compute/detail/sha1.hpp>

#include "../src/util.hpp"
#include "../src/manifest/manifest.hpp"

#include <sched.h>
#include <time.h>
#include <stdio.h>
#include <sys/stat.h>

namespace repo {

using namespace ndn::time;

using std::shared_ptr;
using std::make_shared;
using std::bind;

static const uint64_t DEFAULT_BLOCK_SIZE = 1000;
static const uint64_t DEFAULT_INTEREST_LIFETIME = 4000;
static const uint64_t DEFAULT_FRESHNESS_PERIOD = 10000;
static const uint64_t DEFAULT_CHECK_PERIOD = 1000;
static const size_t PRE_SIGN_DATA_COUNT = 11;

char* file_name;
clock_t start, end;
double time_result;
bool flag = false;

class NdnHcSignPerf : boost::noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    using std::runtime_error::runtime_error;
  };

  NdnHcSignPerf()
    : isSingle(false)
    , useDigestSha256(false)
    , freshnessPeriod(DEFAULT_FRESHNESS_PERIOD)
    , interestLifetime(DEFAULT_INTEREST_LIFETIME)
    , hasTimeout(false)
    , timeout(0)
    , blockSize(DEFAULT_BLOCK_SIZE)
    , insertStream(nullptr)
    , isVerbose(false)
    , m_scheduler(m_face.getIoService())
    , m_timestampVersion(toUnixTimestamp(system_clock::now()).count())
    , m_processId(0)
    , m_checkPeriod(DEFAULT_CHECK_PERIOD)
    , m_currentSegmentNo(0)
    , m_isFinished(false)
    , m_cmdSigner(m_keyChain)
  {
  }

  void signFirstData(ndn::Data& data);

  void
  prepareHashes();

  void
  run();

  void
  signData(ndn::Data& data);

  boost::filesystem::path
  getPath(const Name& name);

  std::string
  sha1Hash(std::string const& key);

public:
  bool isSingle;
  bool useDigestSha256;
  std::string identityForData;
  std::string identityForCommand;
  milliseconds freshnessPeriod;
  milliseconds interestLifetime;
  bool hasTimeout;
  milliseconds timeout;
  size_t blockSize;
  ndn::Name repoPrefix;
  ndn::Name ndnName;
  std::istream* insertStream;
  bool isVerbose;

  //added labry
  std::string str_ndnName;
  std::string str_repoPrefix;

private:
  boost::filesystem::path m_path = "/app/difs-hash/ndnsignedpackets/";
  ndn::Face m_face;
  ndn::Scheduler m_scheduler;
  ndn::KeyChain m_keyChain;
  uint64_t m_timestampVersion;
  uint64_t m_processId;
  milliseconds m_checkPeriod;
  size_t m_currentSegmentNo;
  bool m_isFinished;
  ndn::Name m_dataPrefix;
  std::list<std::array<uint8_t,util::HASH_SIZE>> hashes;

  size_t m_bytes;
  size_t m_firstSize;

  //using DataContainer = std::map<uint64_t, shared_ptr<ndn::Data>>;
  std::list<shared_ptr<ndn::Data>> m_data;
  ndn::security::CommandInterestSigner m_cmdSigner;
};

std::string
NdnHcSignPerf::sha1Hash(std::string const& key)
{
  boost::compute::detail::sha1 sha1;
  sha1.process(key);
  return std::string(sha1);
}

boost::filesystem::path
NdnHcSignPerf::getPath(const Name& name)
{
  auto hash = sha1Hash(name.toUri());
  auto dir1 = hash.substr(0, 2);
  auto dir2 = hash.substr(2, hash.size() - 2);
  return m_path / dir1 / dir2;
}

void
NdnHcSignPerf::run()
{

  std::cout << "hello NdnHcSignPerf with segment_size = " <<blockSize<< std::endl;

  m_dataPrefix = ndnName;

  start = clock();

  struct stat st;
  stat(file_name, &st);
  std::cerr<<"file name is "<<file_name<<std::endl;
  m_bytes = st.st_size;


  std::cout<< "repo name: "<< str_repoPrefix <<std::endl;
  std::cout<< "data name: "<< str_ndnName <<std::endl;
  std::cout << "m_bytes: " << m_bytes << std::endl;
  // no need for ll just make data segments with RSA, ECDSA, or SHA256 
  // make sure m_data has [referenceSegmentNo, referenceSegmentNo + PRE_SIGN_DATA_COUNT] Data
  // Mkae this a loop segmentNo for referenceSegmentNo 

  int referenceSegmentNo = 0;
  int finalSegmentNo = m_bytes/blockSize;

  // while(true) {

  // DataContainer::iterator item = m_data.find(referenceSegmentNo);
  if (referenceSegmentNo == finalSegmentNo) {
    std::cout<< "m_data.end()" <<std::endl;
    //return;
  }

  size_t nDataToPrepare = finalSegmentNo-referenceSegmentNo;
  std::cout<<"f and r:"<<finalSegmentNo<<" "<<referenceSegmentNo;

  prepareHashes();

  auto dataSize = blockSize - util::HASH_SIZE;
  for (size_t i = 0; i < nDataToPrepare && !m_isFinished; ++i) {
    auto segNo = referenceSegmentNo + i;

    //std::cout << "segno: " << segNo << std::endl;
    //std::cout << "hashes size: " << hashes.size() << std::endl;

    uint8_t *buffer = new uint8_t[blockSize];
    std::array<uint8_t,util::HASH_SIZE> hash;
    if (!hashes.empty()) {
      hash = hashes.front();
      hashes.pop_front();
    } else {
      hash = {0};
      m_isFinished = true;
    }


    memcpy(buffer, &hash, util::HASH_SIZE);

    auto toRead = dataSize;
    if (segNo == 0) {
      toRead = m_firstSize;
    }

    auto readSize = boost::iostreams::read(*insertStream,
                                           reinterpret_cast<char*>(buffer + util::HASH_SIZE), toRead);
    if (readSize <= 0) {
      BOOST_THROW_EXCEPTION(Error("Error reading from the input stream"));
    }

    auto data = make_shared<ndn::Data>(Name(m_dataPrefix).appendNumber(m_currentSegmentNo));
    //std::cerr<<"data name is "<<data.getName()<<std::endl;
    //std::cerr<<"data full name "<<data.getFullName() << std::endl;
    if (m_isFinished) {
      std::cout << "Finished" << std::endl;
      data->setFinalBlock(ndn::name::Component::fromSegment(m_currentSegmentNo));
    }

    data->setContent(buffer, toRead + util::HASH_SIZE);
    data->setFreshnessPeriod(freshnessPeriod);
    if(segNo == 0) {
      signFirstData(*data);
    } else {
      signData(*data);
    }

    m_data.push_back(data);

    ++m_currentSegmentNo;
    delete[] buffer;
  }

  
  // if (isVerbose)
  //   std::cerr << "setInterestFilter for " << m_dataPrefix << std::endl;

  // referenceSegmentNo++;
  std::ofstream manifest;
  manifest.open ("/app/difs-hash/manifest.txt");
  manifest << ndnName << std::endl;
  manifest << m_currentSegmentNo;
  manifest.close();
  std::cout<< "ndnName: "<<ndnName <<std::endl;
  std::cout<< "The total number of segments: "<<m_currentSegmentNo <<std::endl;
  // }
  end = clock();
  time_result = (double)(end - start);
  printf("time is %f\n", time_result/CLOCKS_PER_SEC);

  for (size_t i = 0; i < nDataToPrepare; i++) {
    shared_ptr<ndn::Data> data = m_data.front();
    m_data.pop_front();
    // std::cout<<"data.name:"<<data->getName()<<std::endl;
    // std::cout<<"data.uri:"<<data->getName().toUri()<<std::endl;
    //
    boost::filesystem::path fsPath = getPath(data->getName());
    // std::cout<<"fsPath string:"<<fsPath.generic_string()<<std::endl;
    //boost::filesystem::remove(fsPath);
    boost::filesystem::create_directories(fsPath.parent_path());

    std::ofstream outFileData(fsPath.string(), std::ios::binary);
    outFileData.write(
      reinterpret_cast<const char*>(data->wireEncode().wire()),
      data->wireEncode().size());
  }

}


void
NdnHcSignPerf::signData(ndn::Data& data)
{
  if (useDigestSha256) {
    m_keyChain.sign(data, ndn::signingWithSha256());
  }
  else if (identityForData.empty())
    m_keyChain.sign(data);
  else {
    m_keyChain.sign(data, ndn::signingByIdentity(identityForData));
  }
}

void
NdnHcSignPerf::prepareHashes()
{
  int dataSize = blockSize - util::HASH_SIZE;
  std::array<uint8_t,util::HASH_SIZE> hash;
  std::array<uint8_t,util::HASH_SIZE> prevHash;
  uint8_t *buffer = new uint8_t[blockSize];

  int position;
  for (position = dataSize; position < (int)m_bytes ; position += dataSize) {
    if (!hashes.empty()) {
      prevHash = hashes.front();
    }
    memcpy(buffer, prevHash.data(), util::HASH_SIZE);
    // This part is to read from the behind.
    insertStream->seekg(-position, std::ios::end);
    auto readSize = boost::iostreams::read(*insertStream, reinterpret_cast<char*>(buffer + util::HASH_SIZE), dataSize);
    if (readSize <= 0) {
      BOOST_THROW_EXCEPTION(Error("Error reading from the input stream"));
    }

    std::streambuf* buf;
    buf = std::cout.rdbuf();
    std::ostream os(buf);

    //std::cout << "Content: ";
    //os.write(reinterpret_cast<const char *>(buffer), blockSize);
    //std::cout << std::endl;

    //std::ios_base::fmtflags g(std::cout.flags());
    //std::cout << "Content(hex): " << std::hex;
    //for (int i = 0; i < (int)blockSize; i += 1) {
      //printf("%02x", buffer[i]);
    //}
    //std::cout.flags(g);
    //std::cout << std::endl;

    hash = util::calcHash(buffer, blockSize);

    //std::cout << (buffer+util::HASH_SIZE) << std::endl;

    //std::cout << "Hash: " << std::hex;
    //for (const auto& s : hash) {
      //printf("%02x", s);
    //}
    //std::cout << std::endl;
    hashes.push_front(hash);
  }

  // save first block size
  // If position >= m_bytes, only one block is generated and no hash chain
  m_firstSize = m_bytes - (position - dataSize);
  //std::cout << "first data size = " << m_firstSize << std::endl;
  insertStream->seekg(0, std::ios::beg);
}

void
NdnHcSignPerf::signFirstData(ndn::Data& data)
{
if (identityForData.empty())
    m_keyChain.sign(data);
  else {
    m_keyChain.sign(data, ndn::signingByIdentity(identityForData));
  }
}

static void
usage(const char* programName)
{
  std::cerr << "Usage: "
            << programName << " [-u] [-D] [-d] [-s block size] [-i identity] [-I identity] [-x freshness]"
                              " [-l lifetime] [-w timeout] repo-prefix ndn-name filename\n"
            << "\n"
            << "Write a file into a repo.\n"
            << "\n"
            << "  -D: use DigestSha256 signing method instead of SignatureSha256WithRsa\n"
            << "  -i: specify identity used for signing Data\n"
            << "  -I: specify identity used for signing commands\n"
            << "  -x: FreshnessPeriod in milliseconds\n"
            << "  -l: InterestLifetime in milliseconds for each command\n"
            << "  -w: timeout in milliseconds for whole process (default unlimited)\n"
            << "  -s: block size (default 1000)\n"
            << "  -v: be verbose\n"
            << "  repo-prefix: repo command prefix\n"
            << "  ndn-name: NDN Name prefix for written Data\n"
            << "  filename: local file name; \"-\" reads from stdin\n"
            << std::endl;
}

static int
main(int argc, char** argv)
{
  NdnHcSignPerf ndnHcSignPerf;

  int opt;
  while ((opt = getopt(argc, argv, "hDi:I:x:l:w:s:v")) != -1) {
    switch (opt) {
    case 'h':
      usage(argv[0]);
      return 0;
    case 'D':
      ndnHcSignPerf.useDigestSha256 = true;
      break;
    case 'i':
      ndnHcSignPerf.identityForData = std::string(optarg);

      break;
    case 'I':
      ndnHcSignPerf.identityForCommand = std::string(optarg);
      break;
    case 'x':
      try {
        ndnHcSignPerf.freshnessPeriod = milliseconds(boost::lexical_cast<uint64_t>(optarg));
      }
      catch (const boost::bad_lexical_cast&) {
        std::cerr << "-x option should be an integer" << std::endl;;
        return 2;
      }
      break;
    case 'l':
      try {
        ndnHcSignPerf.interestLifetime = milliseconds(boost::lexical_cast<uint64_t>(optarg));
      }
      catch (const boost::bad_lexical_cast&) {
        std::cerr << "-l option should be an integer" << std::endl;;
        return 2;
      }
      break;
    case 'w':
      ndnHcSignPerf.hasTimeout = true;
      try {
        ndnHcSignPerf.timeout = milliseconds(boost::lexical_cast<uint64_t>(optarg));
      }
      catch (const boost::bad_lexical_cast&) {
        std::cerr << "-w option should be an integer" << std::endl;;
        return 2;
      }
      break;
    case 's':
      try {
        ndnHcSignPerf.blockSize = boost::lexical_cast<uint64_t>(optarg);
      }
      catch (const boost::bad_lexical_cast&) {
        std::cerr << "-s option should be an integer.";
        return 1;
      }
      break;
    case 'v':
      ndnHcSignPerf.isVerbose = true;
      break;
    default:
      usage(argv[0]);
      return 2;
    }
  }

  if (argc != optind + 3) {
    usage(argv[0]);
    return 2;
  }

  argc -= optind;
  argv += optind;

  ndnHcSignPerf.str_repoPrefix = argv[0];
  ndnHcSignPerf.repoPrefix = Name(argv[0]);

  ndnHcSignPerf.str_ndnName = argv[1];
  ndnHcSignPerf.ndnName = Name(argv[1]);

  file_name = argv[2];

  if (strcmp(argv[2], "-") == 0) {
    ndnHcSignPerf.insertStream = &std::cin;
    ndnHcSignPerf.run();
  }
  else {
    std::ifstream inputFileStream(argv[2], std::ios::in | std::ios::binary);
    if (!inputFileStream.is_open()) {
      std::cerr << "ERROR: cannot open " << argv[2] << std::endl;
      return 2;
    }

    ndnHcSignPerf.insertStream = &inputFileStream;
    ndnHcSignPerf.run();
  }

  // ndnHcSignPerf MUST NOT be used anymore because .insertStream is a dangling pointer

  return 0;
}

} // namespace repo

int
main(int argc, char** argv)
{
  try {
    return repo::main(argc, argv);
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 1;
  }
}
