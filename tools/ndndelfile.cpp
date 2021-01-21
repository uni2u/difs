#include "../src/repo-command-parameter.hpp"
#include "../src/repo-command-response.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/command-interest-signer.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include "ndndelfile.hpp"

#include <iostream>

#include <boost/lexical_cast.hpp>

namespace repo {

using ndn::Name;
using ndn::Interest;
using ndn::Data;
using ndn::Block;

using std::bind;
using std::placeholders::_1;
using std::placeholders::_2;

static const int MAX_RETRY = 3;


void
NdnDelFile::run()
{
  Name name(m_dataName);

  deleteData(name);

  m_face.processEvents(m_timeout);
}

void
NdnDelFile::deleteData(const Name& name)
{
  // TODO Implement this
  // See ndnputfile

  std::cout << "Delete " << m_dataName << std::endl;


  RepoCommandParameter parameters;
  parameters.setProcessId(0);  // FIXME: set process id properly
  parameters.setName(m_dataName);

  // TODO: send delete command interest
  ndn::Interest commandInterest = generateCommandInterest(m_repoPrefix, "delete", parameters);
  m_face.expressInterest(commandInterest,
    bind(&NdnDelFile::onDeleteCommandResponse, this, _1, _2),
    bind(&NdnDelFile::onDeleteCommandTimeout, this, _1),  // Nack
    bind(&NdnDelFile::onDeleteCommandTimeout, this, _1));
}

void
NdnDelFile::onTimeout(const Interest& interest)
{
  if (m_retryCount++ < MAX_RETRY) {
    deleteData(Name(m_dataName));
    if (m_verbose) {
      std::cerr << "TIMEOUT: retransmit interest for " << interest.getName() << std::endl;
    }
  } else {
    std::cerr << "TIMEOUT: last interest sent" << std::endl
    << "TIMEOUT: abort fetching after " << MAX_RETRY << " times of retry" << std::endl;
  }
}


ndn::Interest
NdnDelFile::generateCommandInterest(const ndn::Name& commandPrefix, const std::string& command,
                                    const RepoCommandParameter& commandParameter)
{
  Name cmd = commandPrefix;
  cmd
    .append(command)
    .append(commandParameter.wireEncode());
  ndn::Interest interest;

  interest = m_cmdSigner.makeCommandInterest(cmd);

  interest.setInterestLifetime(m_interestLifetime);
  interest.setMustBeFresh(true);
  return interest;
}

void
NdnDelFile::onDeleteCommandResponse(const ndn::Interest& interest, const ndn::Data& data)
{
  RepoCommandResponse response(data.getContent().blockFromValue());
  int statusCode = response.getCode();
  if (statusCode == 404) {
    std::cerr << "Manifest not found" << std::endl;
    return;
  }
  else if (statusCode >= 400) {
    std::cerr << "delete command failed with code " << statusCode << interest.getName() << std::endl;
    return;
  }
}

void
NdnDelFile::onDeleteCommandTimeout(const ndn::Interest& interest)
{
  std::cerr << "ERROR: timeout while quering " << interest.getName() << std::endl;
}

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

  NdnDelFile ndnDelFile(repoPrefix, name, verbose, interestLifetime, timeout);

  try
  {
    ndnDelFile.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }

  return 0;
}

}  // namespace repo

int main(int argc, char** argv)
{
  return repo::main(argc, argv);
}
