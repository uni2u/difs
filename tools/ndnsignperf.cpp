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

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/read.hpp>

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

class NdnSignPerf : boost::noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    using std::runtime_error::runtime_error;
  };

  NdnSignPerf()
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

  void
  run();

  void
  signData(ndn::Data& data);


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
  ndn::Face m_face;
  ndn::Scheduler m_scheduler;
  ndn::KeyChain m_keyChain;
  uint64_t m_timestampVersion;
  uint64_t m_processId;
  milliseconds m_checkPeriod;
  size_t m_currentSegmentNo;
  bool m_isFinished;
  ndn::Name m_dataPrefix;

  size_t m_bytes;

  using DataContainer = std::map<uint64_t, shared_ptr<ndn::Data>>;
  DataContainer m_data;
  ndn::security::CommandInterestSigner m_cmdSigner;
};


void
NdnSignPerf::run()
{

  std::cout << "hello NdnSignPerf with segment_size = " <<blockSize<< std::endl;

  m_dataPrefix = ndnName;

   start = clock();
/*
  insertStream->seekg(0, std::ios::beg);
  auto beginPos = insertStream->tellg();
  insertStream->seekg(0, std::ios::end);
  m_bytes = insertStream->tellg() - beginPos;
  printf("1st m_bytes %d\n", m_bytes);
  insertStream->seekg(0, std::ios::beg);
*/
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
    return;
  }

  size_t nDataToPrepare = finalSegmentNo-referenceSegmentNo;

  for (size_t i = 0; i < nDataToPrepare && !m_isFinished; ++i) {
    uint8_t *buffer = new uint8_t[blockSize];
    auto readSize = boost::iostreams::read(*insertStream,
                                           reinterpret_cast<char*>(buffer), blockSize);
    if (readSize <= 0) {
      BOOST_THROW_EXCEPTION(Error("Error reading from the input stream"));
    }

    auto data = make_shared<ndn::Data>(Name(m_dataPrefix).appendSegment(m_currentSegmentNo));

    if (insertStream->peek() == std::istream::traits_type::eof()) {
      data->setFinalBlock(ndn::name::Component::fromSegment(m_currentSegmentNo));
      m_isFinished = true;
    }

    data->setContent(buffer, readSize);
    data->setFreshnessPeriod(freshnessPeriod);
    signData(*data);

    m_data.insert(std::make_pair(m_currentSegmentNo, data));

    ++m_currentSegmentNo;
    delete[] buffer;
  }

  // if (isVerbose)
  //   std::cerr << "setInterestFilter for " << m_dataPrefix << std::endl;

  // referenceSegmentNo++;
  std::cout<< "nDataToPrepared: "<<nDataToPrepare <<std::endl;
  // }
  end = clock();
  time_result = (double)(end - start);
  printf("time is %f\n", time_result/CLOCKS_PER_SEC);

}


void
NdnSignPerf::signData(ndn::Data& data)
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
  NdnSignPerf ndnSignPerf;

  int opt;
  while ((opt = getopt(argc, argv, "hDi:I:x:l:w:s:v")) != -1) {
    switch (opt) {
    case 'h':
      usage(argv[0]);
      return 0;
    case 'D':
      ndnSignPerf.useDigestSha256 = true;
      break;
    case 'i':
      ndnSignPerf.identityForData = std::string(optarg);

      break;
    case 'I':
      ndnSignPerf.identityForCommand = std::string(optarg);
      break;
    case 'x':
      try {
        ndnSignPerf.freshnessPeriod = milliseconds(boost::lexical_cast<uint64_t>(optarg));
      }
      catch (const boost::bad_lexical_cast&) {
        std::cerr << "-x option should be an integer" << std::endl;;
        return 2;
      }
      break;
    case 'l':
      try {
        ndnSignPerf.interestLifetime = milliseconds(boost::lexical_cast<uint64_t>(optarg));
      }
      catch (const boost::bad_lexical_cast&) {
        std::cerr << "-l option should be an integer" << std::endl;;
        return 2;
      }
      break;
    case 'w':
      ndnSignPerf.hasTimeout = true;
      try {
        ndnSignPerf.timeout = milliseconds(boost::lexical_cast<uint64_t>(optarg));
      }
      catch (const boost::bad_lexical_cast&) {
        std::cerr << "-w option should be an integer" << std::endl;;
        return 2;
      }
      break;
    case 's':
      try {
        ndnSignPerf.blockSize = boost::lexical_cast<uint64_t>(optarg);
      }
      catch (const boost::bad_lexical_cast&) {
        std::cerr << "-s option should be an integer.";
        return 1;
      }
      break;
    case 'v':
      ndnSignPerf.isVerbose = true;
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

  ndnSignPerf.str_repoPrefix = argv[0];
  ndnSignPerf.repoPrefix = Name(argv[0]);

  ndnSignPerf.str_ndnName = argv[1];
  ndnSignPerf.ndnName = Name(argv[1]);

  file_name = argv[2];

  if (strcmp(argv[2], "-") == 0) {
    ndnSignPerf.insertStream = &std::cin;
    ndnSignPerf.run();
  }
  else {
    std::ifstream inputFileStream(argv[2], std::ios::in | std::ios::binary);
    if (!inputFileStream.is_open()) {
      std::cerr << "ERROR: cannot open " << argv[2] << std::endl;
      return 2;
    }

    ndnSignPerf.insertStream = &inputFileStream;
    ndnSignPerf.run();
  }

  // ndnSignPerf MUST NOT be used anymore because .insertStream is a dangling pointer

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
