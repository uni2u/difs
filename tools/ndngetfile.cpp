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

// #include <ndn-cxx/data.hpp>
// #include <ndn-cxx/face.hpp>
// #include <ndn-cxx/interest.hpp>

#include <fstream>
#include <iostream>
#include <unistd.h>

#include <boost/lexical_cast.hpp>

#include "difs.hpp"

int
usage(const char* programName)
{
  std::cerr << "Usage: "
            << programName << " [-v] [-l lifetime] [-w timeout] [-o filename] \n"
            << "\n"
            << "  -v: be verbose\n"
            << "  -f: set forwardingHint\n"
            << "  -l: InterestLifetime in milliseconds\n"
            << "  -w: timeout in milliseconds for whole process (default unlimited)\n"
            << "  -o: write to local file name instead of stdout\n"
            << "  ndn-name: NDN Name prefix for Data to be read\n"
            << std::endl;
  return 1;
}

int
main(int argc, char** argv)
{
  std::string repoPrefix;
  std::string name, forwardingHint;
  const char* outputFile = nullptr;
  bool verbose = false;
  int interestLifetime = 4000;  // in milliseconds
  int timeout = 0;  // in milliseconds

  int opt;
  while ((opt = getopt(argc, argv, "hvf:l:w:o:")) != -1) {
    switch (opt) {
    case 'h':
      usage(argv[0]);
      return 0;
    case 'v':
      verbose = true;
      break;
    case 'f':
      forwardingHint = optarg;
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

  if (optind + 2 != argc) {
    return usage(argv[0]);
  }

  repoPrefix = argv[optind];
  name = argv[optind+1];

  if (name.empty() || repoPrefix.empty())
  {
    return usage(argv[0]);
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

  difs::DIFS difs(repoPrefix, interestLifetime, timeout, verbose);

  if(!forwardingHint.empty()) {
    ndn::Delegation d;
    d.name = ndn::Name(forwardingHint);
    difs.setForwardingHint(ndn::DelegationList{d});
  }

  difs.getFile(name, os);

  try
  {
    difs.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }

  return 0;
}