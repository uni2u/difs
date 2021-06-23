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
  bool verbose;
  int interestLifetime = 4000;  // in milliseconds
  int timeout = 0;  // in milliseconds

  int opt;
  while ((opt = getopt(argc, argv, "vf:l:w:o:")) != -1)
  {
    switch (opt) {
      case 'v':
        verbose = true;
        break;
      case 'f':
        forwardingHint = optarg;
        break;
      case 'l':
        try
        {
          interestLifetime = boost::lexical_cast<int>(optarg);
        }
        catch (const boost::bad_lexical_cast&)
        {
          std::cerr << "ERROR: -l option should be an integer." << std::endl;
          return 1;
        }
        interestLifetime = std::max(interestLifetime, 0);
        break;
      case 'w':
        try
        {
          timeout = boost::lexical_cast<int>(optarg);
        }
        catch (const boost::bad_lexical_cast&)
        {
          std::cerr << "ERROR: -w option should be an integer." << std::endl;
          return 1;
        }
        timeout = std::max(timeout, 0);
        break;
      default:
        return usage(argv[0]);
    }
  }

  if (optind + 1 != argc) {
    return usage(argv[0]);
  }

  repoPrefix = argv[optind];

  if (repoPrefix.empty())
  {
    return usage(argv[0]);
  }

  difs::DIFS difs(repoPrefix);

  if(!forwardingHint.empty()) {
    ndn::Delegation d;
    d.name = ndn::Name(forwardingHint);
    difs.setForwardingHint(ndn::DelegationList{d});
  }

  difs.getKeySpaceInfo();

  try
  {
    difs.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << "ERROR: " << e.what() << std::endl; }
}



