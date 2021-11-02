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

#include <iostream>

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <fstream>
#include <string>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/read.hpp>
#include <boost/noncopyable.hpp>

#include "difs.hpp"

void
usage(const char* programName)
{
  std::cerr << "Usage: "
            << programName << " [-u] [-D] [-d] [-s block size] [-i identity] [-I identity] [-x freshness]"
                              " [-l lifetime] [-w timeout] REPO-PREFIX NDN-NAME KEY FILENAME\n"
            << "\n"
            << "Write a file into a repo.\n"
            << "\n"
            << "  -D: use DigestSha256 signing method instead of SignatureSha256WithRsa\n"
            << "  -f: set forwardingHint\n"
            << "  -n: set nodePrefix(backwardingHint)\n"
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

int
main(int argc, char** argv)
{ 
  ndn::time::milliseconds interestLifetime, timeout, freshnessPeriod;
  bool useDigestSha256, hasTimeout, verbose;
  std::string identityForData, identityForCommand;
  std::string repoPrefix, dataPrefix;
  std::string difsKey, forwardingHint, nodePrefix;
  std::istream* insertStream;
  size_t blockSize;
  
  int opt;
  while ((opt = getopt(argc, argv, "hDf:n:i:I:x:l:w:s:v")) != -1) {
    switch (opt) {
    case 'h':
      usage(argv[0]);
      return 0;
    case 'D':
      useDigestSha256 = true;
      break;
    case 'f':
      forwardingHint = optarg;
      break;
    case 'n':
      nodePrefix = optarg;
      break;
    case 'i':
      identityForData = std::string(optarg);
      break;
    case 'I':
      identityForCommand = std::string(optarg);
      break;
    case 'x':
      try {
        freshnessPeriod = ndn::time::milliseconds(boost::lexical_cast<uint64_t>(optarg));
      }
      catch (const boost::bad_lexical_cast&) {
        std::cerr << "-x option should be an integer" << std::endl;;
        return 2;
      }
      break;
    case 'l':
      try {
        interestLifetime = ndn::time::milliseconds(boost::lexical_cast<uint64_t>(optarg));
      }
      catch (const boost::bad_lexical_cast&) {
        std::cerr << "-l option should be an integer" << std::endl;;
        return 2;
      }
      break;
    case 'w':
      hasTimeout = true;
      try {
        timeout = ndn::time::milliseconds(boost::lexical_cast<uint64_t>(optarg));
      }
      catch (const boost::bad_lexical_cast&) {
        std::cerr << "-w option should be an integer" << std::endl;;
        return 2;
      }
      break;
    case 's':
      try {
        blockSize = boost::lexical_cast<uint64_t>(optarg);
      }
      catch (const boost::bad_lexical_cast&) {
        std::cerr << "-s option should be an integer.";
        return 1;
      }
      break;
    case 'v':
      verbose = true;
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

  repoPrefix = argv[0];
  dataPrefix = argv[1];

  std::ifstream inputFileStream;
  if (strcmp(argv[2], "-") == 0) {
    insertStream = &std::cin;
  }
  else {
    inputFileStream.open(argv[2], std::ios::in | std::ios::binary);
    if (!inputFileStream.is_open()) {
      std::cerr << "ERROR: cannot open " << argv[3] << std::endl;
      return 2;
    }

    insertStream = &inputFileStream;
  }

  difs::DIFS difs(repoPrefix);

  if(!nodePrefix.empty()) {
    ndn::Delegation d;
    d.name = ndn::Name(nodePrefix);
    difs.setNodePrefix(ndn::DelegationList{d});
  }

  if(!forwardingHint.empty()) {
    ndn::Delegation d;
    d.name = ndn::Name(forwardingHint);
    difs.setForwardingHint(ndn::DelegationList{d});
  }

  difs.putFile(ndn::Name(dataPrefix), *insertStream, identityForData, identityForCommand);
  //difs.putFile(dataPrefix, *insertStream);

  try
  {
    difs.run();
  }
  catch (const std::exception &e)
  {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }

  return 0;
}