#include <iostream>
#include <boost/lexical_cast.hpp>

#include "difs.hpp"

using ndn::Name;
using ndn::Interest;
using ndn::Data;
using ndn::Block;

using std::bind;
using std::placeholders::_1;
using std::placeholders::_2;

static const int MAX_RETRY = 3;

int
usage(const std::string& filename)
{
  std::cerr << "Usage: \n    "
            << filename << " [-v] [-l lifetime] [-w timeout] repo-name ndn-name\n\n"
            << "-v: be verbose\n"
            << "-l: InterestLifetime in milliseconds\n"
            << "-w: timeout in milliseconds for whole process (default unlimited)\n"
            << "repo-prefix: repo command prefix\n"
            << "ndn-name: NDN Name prefix for Data to be read\n";
  return 1;
}

int
main(int argc, char** argv)
{
  std::string repoPrefix;
  std::string name;
  bool verbose = false;
  int interestLifetime = 4000;  // in milliseconds
  int timeout = 0;  // in milliseconds

  int opt;
  while ((opt = getopt(argc, argv, "vl:w:o:")) != -1)
  {
    switch (opt) {
      case 'v':
        verbose = true;
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

  if (optind + 2 != argc) {
    return usage(argv[0]);
  }

  repoPrefix = argv[optind];
  name = argv[optind+1];

  if (name.empty() || repoPrefix.empty())
  {
    return usage(argv[0]);
  }

  difs::DIFS difs(repoPrefix, verbose, interestLifetime, timeout);
  difs.deleteFile(name);

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