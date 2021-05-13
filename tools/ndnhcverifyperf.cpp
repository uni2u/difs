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
#include <ndn-cxx/security/validator-config.hpp>
#include <ndn-cxx/security/validation-error.hpp>
#include <ndn-cxx/security/validator.hpp>

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
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ptree.hpp>


#include "../src/manifest/manifest.hpp"
#include "util.hpp"

#include <sched.h>
#include <time.h>
#include <stdio.h>
#include <sys/stat.h>

namespace repo {

using namespace ndn::time;

using std::shared_ptr;
using std::make_shared;
using std::bind;

static bool m_isFirst;
static std::array<uint8_t, util::HASH_SIZE> prevHash;

static const uint64_t DEFAULT_BLOCK_SIZE = 1000;
static const uint64_t DEFAULT_INTEREST_LIFETIME = 4000;
static const uint64_t DEFAULT_FRESHNESS_PERIOD = 10000;
static const uint64_t DEFAULT_CHECK_PERIOD = 1000;
static const size_t PRE_SIGN_DATA_COUNT = 11;
boost::filesystem::path m_path = "/app/difs-hash/ndnsignedpackets/";
using DataContainer = std::map<uint64_t, shared_ptr<ndn::Data>>;
std::list<shared_ptr<ndn::Data>> m_data;

char* file_name;
clock_t start, end;
double time_result;
bool flag = false;

class NdnHcVeriPerf : boost::noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    using std::runtime_error::runtime_error;
  };

  NdnHcVeriPerf()
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
  bool 
  verifyData(const ndn::Data& data);

  std::string
  sha1Hash(std::string const& key);

  boost::filesystem::path
  getPath(const Name& name);

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
//   ndn::security::Validator validator;
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
  ndn::security::CommandInterestSigner m_cmdSigner;
};


std::string
NdnHcVeriPerf::sha1Hash(std::string const& key)
{
  boost::compute::detail::sha1 sha1;
  sha1.process(key);
  return std::string(sha1);
}

boost::filesystem::path
NdnHcVeriPerf::getPath(const Name& name)
{
  auto hash = sha1Hash(name.toUri());
  auto dir1 = hash.substr(0, 2);
  auto dir2 = hash.substr(2, hash.size() - 2);
  return m_path / dir1 / dir2;
}

bool 
NdnHcVeriPerf::verifyData(const ndn::Data& data)
{
  int32_t tlvType = data.getSignatureInfo().getSignatureType();
  //NDN_LOG_DEBUG("verifyData: " << tlvType);
  bool ret;
  auto content = data.getContent();

  if (m_isFirst) {
    m_isFirst = false;
    ret = true;
  } else {
    ret = util::verifyHash(content.value(), content.value_size(), prevHash);
  }
  for (int i = 0; i < util::HASH_SIZE; i += 1) {
    prevHash[i] = content.value()[i];
  }

  return ret;
}

void
NdnHcVeriPerf::run()
{
  ndn::security::ValidatorConfig m_validator(m_face);
  boost::property_tree::ptree validatorNode;

    std::string configPath = "/app/difs-hash/repo-veri.conf";
    std::ifstream fin(configPath.c_str());
    if (!fin.is_open())
        std::cout<<"failed to open configuration file '" + configPath + "'";

    using namespace boost::property_tree;
    ptree propertyTree;
    try {
        read_info(fin, propertyTree);
    }
    catch (const ptree_error& e) {
        std::cout<<"failed to read configuration file '" + configPath + "'";
    }

  ptree repoConf = propertyTree.get_child("repo");
  validatorNode = repoConf.get_child("validator");

  m_validator.load(validatorNode, configPath);//

  std::cout << "hello NdnHcVeriPerf with segment_size = " <<blockSize<< std::endl;

  m_dataPrefix = ndnName;


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

  std::ifstream manifest ("manifest.txt");
  std::string line;
  std::string tmpName;
  int total_number_of_segments;
  if (manifest.is_open()) {
    getline (manifest,line);
    tmpName = line;
    getline(manifest, line);
    total_number_of_segments = std::stoi(line);
    manifest.close();

    std::cout<<"manifest:"<<tmpName << " "<<total_number_of_segments << std::endl;
  }

   

  for(int i = 0; i < total_number_of_segments; i++) {
      //std::cout<<i<<" ";
      Name name = tmpName;
      name.appendNumber(i);
      auto fsPath = getPath(name);
      boost::filesystem::ifstream inFileData(fsPath, std::ifstream::binary);
      if (!inFileData.is_open()) {
          std::cout<<"Cannot open the file!"<<std::endl;
          std::cout<<fsPath.generic_string()<<std::endl;
          break;
          //return;
      }
      auto data = std::make_shared<ndn::Data>();

      inFileData.seekg(0, inFileData.end);
      int length = inFileData.tellg();
      inFileData.seekg(0, inFileData.beg);

      char * buffer = new char [length];
      inFileData.read(buffer, length);

      data->wireDecode(Block(reinterpret_cast<const uint8_t*>(buffer), length));
      //std::cout<<data->getName()<<std::endl;
      //ata->setName("failed");
      m_data.push_back(data);
      inFileData.close();
    }
  //
  start = clock();

  for(int i = 0; i < total_number_of_segments; i++) {
    shared_ptr<ndn::Data> data = m_data.front();
    m_data.pop_front();
    m_validator.validate(*data,
      bind([] { }),
      bind([] { std::cout<<">Validation have failed"<<std::endl; }));

    if(data->getSignatureInfo().getSignatureType() != tlv::DigestSha256) {
      std::cout<<"This is first of hash chain."<<std::endl;
      m_isFirst = true;
    }

    if (!verifyData(*data)) {
      std::cout<<"Error verifying hash chain";
    }
  }

  std::cout<<"finishing up!"<<std::endl;
  std::cout<<"m_data.size()"<<m_data.size()<<std::endl;
  end = clock();
  time_result = (double)(end - start);
  printf("time is %f\n", time_result/CLOCKS_PER_SEC);
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
  NdnHcVeriPerf ndnHcVeriPerf;

  int opt;
  while ((opt = getopt(argc, argv, "hDi:I:x:l:w:s:v")) != -1) {
    switch (opt) {
    case 'h':
      usage(argv[0]);
      return 0;
    case 'D':
      ndnHcVeriPerf.useDigestSha256 = true;
      break;
    case 'i':
      ndnHcVeriPerf.identityForData = std::string(optarg);

      break;
    case 'I':
      ndnHcVeriPerf.identityForCommand = std::string(optarg);
      break;
    case 'x':
      try {
        ndnHcVeriPerf.freshnessPeriod = milliseconds(boost::lexical_cast<uint64_t>(optarg));
      }
      catch (const boost::bad_lexical_cast&) {
        std::cerr << "-x option should be an integer" << std::endl;;
        return 2;
      }
      break;
    case 'l':
      try {
        ndnHcVeriPerf.interestLifetime = milliseconds(boost::lexical_cast<uint64_t>(optarg));
      }
      catch (const boost::bad_lexical_cast&) {
        std::cerr << "-l option should be an integer" << std::endl;;
        return 2;
      }
      break;
    case 'w':
      ndnHcVeriPerf.hasTimeout = true;
      try {
        ndnHcVeriPerf.timeout = milliseconds(boost::lexical_cast<uint64_t>(optarg));
      }
      catch (const boost::bad_lexical_cast&) {
        std::cerr << "-w option should be an integer" << std::endl;;
        return 2;
      }
      break;
    case 's':
      try {
        ndnHcVeriPerf.blockSize = boost::lexical_cast<uint64_t>(optarg);
      }
      catch (const boost::bad_lexical_cast&) {
        std::cerr << "-s option should be an integer.";
        return 1;
      }
      break;
    case 'v':
      ndnHcVeriPerf.isVerbose = true;
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

  ndnHcVeriPerf.str_repoPrefix = argv[0];
  ndnHcVeriPerf.repoPrefix = Name(argv[0]);

  ndnHcVeriPerf.str_ndnName = argv[1];
  ndnHcVeriPerf.ndnName = Name(argv[1]);

  file_name = argv[2];

  if (strcmp(argv[2], "-") == 0) {
    ndnHcVeriPerf.insertStream = &std::cin;
    ndnHcVeriPerf.run();
  }
  else {
    std::ifstream inputFileStream(argv[2], std::ios::in | std::ios::binary);
    if (!inputFileStream.is_open()) {
      std::cerr << "ERROR: cannot open " << argv[2] << std::endl;
      return 2;
    }

    ndnHcVeriPerf.insertStream = &inputFileStream;
    ndnHcVeriPerf.run();
  }

  // ndnHcVeriPerf MUST NOT be used anymore because .insertStream is a dangling pointer

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
