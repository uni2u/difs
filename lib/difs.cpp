#include "repo-command-parameter.hpp"
#include "repo-command-response.hpp"
#include "util.hpp"

#include "manifest/manifest.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/command-interest-signer.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include "difs.hpp"
#include "consumer.hpp"

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <string>

#include <boost/asio.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/read.hpp>
#include <boost/lexical_cast.hpp>

static const uint64_t DEFAULT_BLOCK_SIZE = 1000;
static const uint64_t DEFAULT_INTEREST_LIFETIME = 4000;
static const uint64_t DEFAULT_FRESHNESS_PERIOD = 10000;
static const uint64_t DEFAULT_CHECK_PERIOD = 1000;
static const size_t PRE_SIGN_DATA_COUNT = 11;

namespace difs {

using ndn::Name;
using ndn::Interest;
using ndn::Data;
using ndn::Block;

using std::bind;
using std::placeholders::_1;
using std::placeholders::_2;

using namespace repo;

static const int MAX_RETRY = 3;

void
DIFS::run()
{
  m_face.processEvents();
}

// Set

void
DIFS::setRepoPrefix(ndn::Name repoPrefix) {
  m_repoPrefix = repoPrefix;
}

void
DIFS::setTimeOut(ndn::time::milliseconds timeout) {
  m_timeout = timeout;
}

void
DIFS::setInterestLifetime(ndn::time::milliseconds interestLifetime) {
  m_interestLifetime = interestLifetime;
}

void
DIFS::setFreshnessPeriod(ndn::time::milliseconds freshnessPeriod) {
  m_freshnessPeriod = freshnessPeriod;
}

void
DIFS::setHasTimeout(bool hasTimeout) {
  m_hasTimeout = hasTimeout;
}

void
DIFS::setVerbose(bool verbose) {
  m_verbose = verbose;
}

void
DIFS::setUseDigestSha256(bool useDigestSha256) {
  m_useDigestSha256 = useDigestSha256;
}

void
DIFS::setBlockSize(size_t blockSize) {
  m_blockSize = blockSize;
}

void
DIFS::setIdentityForData(std::string identityForData) {
  m_identityForData = identityForData;
}

void
DIFS::setIdentityForCommand(std::string identityForCommand) {
  m_identityForCommand = identityForCommand;
}

// Delete
void
DIFS::deleteFile(const Name& data_name)
{
  RepoCommandParameter parameter;
  parameter.setProcessId(0);  // FIXME: set process id properly
  parameter.setName(data_name);

  Name cmd = m_common_name;
  cmd.append("delete")
    .append(parameter.wireEncode());

  ndn::Interest commandInterest = m_cmdSigner.makeCommandInterest(cmd);
  commandInterest.setInterestLifetime(m_interestLifetime);
  commandInterest.setMustBeFresh(true);

  std::cout << commandInterest << std::endl;
  m_face.expressInterest(commandInterest,
                        std::bind(&DIFS::onDeleteCommandResponse, this, _1, _2),
                        std::bind(&DIFS::onDeleteCommandNack, this, _1), // Nack
                        std::bind(&DIFS::onDeleteCommandTimeout, this, _1));
}

void
DIFS::onDeleteCommandResponse(const ndn::Interest& interest, const ndn::Data& data)
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
DIFS::onDeleteCommandTimeout(const Interest& interest)
{
  if (m_retryCount++ < MAX_RETRY) {
    deleteFile(interest.getName());
    if (m_verbose) {
      std::cerr << "TIMEOUT: retransmit interest for " << interest.getName() << std::endl;
    }
  } else {
    std::cerr << "TIMEOUT: last interest sent" << std::endl
    << "TIMEOUT: abort fetching after " << MAX_RETRY << " times of retry" << std::endl;
  }
}

void
DIFS::onDeleteCommandNack(const Interest& interest)
{
  if (m_retryCount++ < MAX_RETRY) {
    deleteFile(interest.getName());
    if (m_verbose) {
      std::cerr << "NACK: retransmit interest for " << interest.getName() << std::endl;
    }
  } else {
    std::cerr << "NACK: last interest sent" << std::endl
    << "NACK: abort fetching after " << MAX_RETRY << " times of retry" << std::endl;
  }
}

// Get

void
DIFS::getFile(const Name& data_name, std::ostream& os)
{
  std::cout << data_name << std::endl;
  RepoCommandParameter parameter;
  //parameter.setProcessId(0);  // FIXME: set process id properly
  parameter.setName(data_name);

  m_os = &os;
  Name cmd = Name("get");
  cmd.append(parameter.wireEncode());

  ndn::Interest commandInterest = m_cmdSigner.makeCommandInterest(cmd);

  commandInterest.setInterestLifetime(m_interestLifetime);
  //commandInterest.setMustBeFresh(true);

  m_face.expressInterest(commandInterest,
                        std::bind(&DIFS::onGetCommandResponse, this, _1, _2),
                        std::bind(&DIFS::onGetCommandNack, this, _1),
                        std::bind(&DIFS::onGetCommandTimeout, this, _1));
}

void
DIFS::onGetCommandResponse(const Interest& interest, const Data& data)
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

  repo::Manifest manifest = Manifest::fromJson(json);

  Consumer consumer(manifest, *m_os);
  consumer.fetch();
}

void
DIFS::onGetCommandTimeout(const Interest& interest)
{
  if (m_retryCount++ < MAX_RETRY) {
    getFile(interest.getName(), *m_os);
    if (m_verbose) {
      std::cerr << "TIMEOUT: retransmit interest for " << interest.getName() << std::endl;
    }
  } else {
    std::cerr << "TIMEOUT: last interest sent" << std::endl
    << "TIMEOUT: abort fetching after " << MAX_RETRY << " times of retry" << std::endl;
  }
}

void
DIFS::onGetCommandNack(const Interest& interest)
{
  if (m_retryCount++ < MAX_RETRY) {
    getFile(interest.getName(), *m_os);
    if (m_verbose) {
      std::cerr << "TIMEOUT: retransmit interest for " << interest.getName() << std::endl;
    }
  } else {
    std::cerr << "TIMEOUT: last interest sent" << std::endl
    << "TIMEOUT: abort fetching after " << MAX_RETRY << " times of retry" << std::endl;
  }
}

// Put

void
DIFS::putFile(const ndn::Name& ndnName, std::istream& is)
{
  m_dataPrefix = ndnName;
  m_ndnName = ndnName;

  m_insertStream = &is;
  m_insertStream->seekg(0, std::ios::beg);
  auto beginPos = m_insertStream->tellg();
  m_insertStream->seekg(0, std::ios::end);
  m_bytes = m_insertStream->tellg() - beginPos;
  m_insertStream->seekg(0, std::ios::beg);

  if (m_verbose)
    std::cerr << "setInterestFilter for " << m_dataPrefix << std::endl;

  m_face.setInterestFilter(m_dataPrefix,
                           bind(&DIFS::onPutFileInterest, this, _1, _2),
                           bind(&DIFS::onPutFileRegisterSuccess, this, _1),
                           bind(&DIFS::onPutFileRegisterFailed, this, _1, _2));

  if (m_hasTimeout)
    m_scheduler.schedule(timeout, [this] { putFileStopProcess(); });
}

void
DIFS::onPutFileInterest(const ndn::Name& prefix, const ndn::Interest& interest)
{
  if (interest.getName().size() == prefix.size()) {
    putFileSendManifest(prefix, interest);
    return;
  }

  uint64_t segmentNo;
  try {
    ndn::Name::Component segmentComponent = interest.getName().get(prefix.size());
    segmentNo = segmentComponent.toSegment();
  }
  catch (const tlv::Error& e) {
    if (m_verbose) {
      std::cerr << "Error processing incoming interest " << interest << ": "
                << e.what() << std::endl;
    }
    return;
  }

  putFilePrepareNextData(segmentNo);

  DataContainer::iterator item = m_data.find(segmentNo);
  if (item == m_data.end()) {
    if (m_verbose) {
      std::cerr << "Requested segment [" << segmentNo << "] does not exist" << std::endl;
    }
    return;
  }

  if (m_isFinished) {
    uint64_t final = m_currentSegmentNo - 1;
    item->second->setFinalBlock(ndn::name::Component::fromSegment(final));
  }
  m_face.put(*item->second);
}

void
DIFS::onPutFileRegisterSuccess(const Name& prefix)
{
  putFileStartInsertCommand();
}

void
DIFS::onPutFileRegisterFailed(const ndn::Name& prefix, const std::string& reason)
{
  BOOST_THROW_EXCEPTION(std::runtime_error("onRegisterFailed: " + reason));
}

void
DIFS::putFileStartInsertCommand()
{
  RepoCommandParameter parameters;
  parameters.setName(m_dataPrefix);

  Name cmd = m_repoPrefix;
  cmd
    .append("insert")
    .append(parameters.wireEncode());

  ndn::Interest commandInterest;
  if (m_identityForCommand.empty())
    commandInterest = m_cmdSigner.makeCommandInterest(cmd);
  else {
    commandInterest = m_cmdSigner.makeCommandInterest(cmd, ndn::signingByIdentity(m_identityForCommand));
  }

  commandInterest.setInterestLifetime(m_interestLifetime);

  m_face.expressInterest(commandInterest,
                         bind(&DIFS::onPutFileInsertCommandResponse, this, _1, _2),
                         bind(&DIFS::onPutFileInsertCommandTimeout, this, _1), // Nack
                         bind(&DIFS::onPutFileInsertCommandNack, this, _1));
}

void
DIFS::onPutFileInsertCommandResponse(const ndn::Interest& interest, const ndn::Data& data)
{
  RepoCommandResponse response(data.getContent().blockFromValue());
  auto statusCode = response.getCode();
  if (statusCode >= 400) {
    BOOST_THROW_EXCEPTION(std::runtime_error("insert command failed with code: " + 
                                              boost::lexical_cast<std::string>(statusCode)));
  }
  m_processId = response.getProcessId();

  m_scheduler.schedule(m_checkPeriod, [this] { putFileStartCheckCommand(); });
}

void
DIFS::onPutFileInsertCommandTimeout(const ndn::Interest& interest)
{
	if(m_retryCount++ < MAX_RETRY) {
		putFileStartInsertCommand();
    if (m_verbose) {
      std::cerr << "TIMEOUT: retransmit interest for " << interest.getName() << std::endl;
    }
	} else {
		std::cerr << "TIMEOUT: last interest sent" << std::endl
		<< "TIMEOUT: abort fetching after " << MAX_RETRY << " times of retry" << std::endl;
	}
}

void
DIFS::onPutFileInsertCommandNack(const ndn::Interest& interest)
{
	if(m_retryCount++ < MAX_RETRY) {
		putFileStartInsertCommand();
    if (m_verbose) {
      std::cerr << "TIMEOUT: retransmit interest for " << interest.getName() << std::endl;
    }
	} else {
		std::cerr << "NACK: last interest sent" << std::endl
		<< "NACK: abort fetching after " << MAX_RETRY << " times of retry" << std::endl;
	}
}

void
DIFS::putFileStartCheckCommand()
{
  auto parameter = RepoCommandParameter();
  parameter.setName(m_ndnName);
  Name cmd = m_repoPrefix;
  cmd
    .append("insert check")
    .append(parameter.setProcessId(m_processId).wireEncode());

  ndn::Interest checkInterest;
  if (m_identityForCommand.empty())
    checkInterest = m_cmdSigner.makeCommandInterest(cmd);
  else {
    checkInterest = m_cmdSigner.makeCommandInterest(cmd, ndn::signingByIdentity(m_identityForCommand));
  }

  checkInterest.setInterestLifetime(m_interestLifetime);

  m_face.expressInterest(checkInterest,
                         bind(&DIFS::onPutFileCheckCommandResponse, this, _1, _2),
                         bind(&DIFS::onPutFileCheckCommandTimeout, this, _1), // Nack
                         bind(&DIFS::onPutFileCheckCommandTimeout, this, _1));
}

void
DIFS::onPutFileCheckCommandResponse(const ndn::Interest& interest, const ndn::Data& data)
{
  RepoCommandResponse response(data.getContent().blockFromValue());
  auto statusCode = response.getCode();
  if (statusCode >= 400) {
    BOOST_THROW_EXCEPTION(std::runtime_error("Insert check command failed with code: " + 
                                              boost::lexical_cast<std::string>(statusCode)));
  }

  if (m_isFinished) {
    uint64_t insertCount = response.getInsertNum();

    // Technically, the check should not infer, but directly has signal from repo that
    // write operation has been finished

    if (insertCount == m_currentSegmentNo) {
      m_face.getIoService().stop();
      return;
    }
  }

  m_scheduler.schedule(m_checkPeriod, [this] { putFileStartCheckCommand(); });
}

void
DIFS::onPutFileCheckCommandTimeout(const ndn::Interest& interest)
{
	if(m_retryCount++ < MAX_RETRY) {
		putFileStartCheckCommand();
    if (m_verbose) {
      std::cerr << "TIMEOUT: retransmit interest for " << interest.getName() << std::endl;
    }
	} else {
		std::cerr << "TIMEOUT: last interest sent" << std::endl
		<< "TIMEOUT: abort fetching after " << MAX_RETRY << " times of retry" << std::endl;
	}
}

void
DIFS::onPutFileCheckCommandNack(const ndn::Interest& interest)
{
	if(m_retryCount++ < MAX_RETRY) {
		putFileStartCheckCommand();
    if (m_verbose) {
      std::cerr << "TIMEOUT: retransmit interest for " << interest.getName() << std::endl;
    }
	} else {
		std::cerr << "NACK: last interest sent" << std::endl
		<< "NACK: abort fetching after " << MAX_RETRY << " times of retry" << std::endl;
	}
}

void
DIFS::putFilePrepareNextData(uint64_t referenceSegmentNo)
{
  if (m_isFinished)
    return;

  size_t nDataToPrepare = PRE_SIGN_DATA_COUNT;

  if (!m_data.empty()) {
    uint64_t maxSegmentNo = m_data.rbegin()->first;

    if (maxSegmentNo - referenceSegmentNo >= nDataToPrepare) {
      // nothing to prepare
      return;
    }

    nDataToPrepare -= maxSegmentNo - referenceSegmentNo;
  }

  for (size_t i = 0; i < nDataToPrepare && !m_isFinished; ++i) {
    uint8_t *buffer = new uint8_t[m_blockSize];
    auto readSize = boost::iostreams::read(*m_insertStream,
                                           reinterpret_cast<char*>(buffer), m_blockSize);
    if (readSize <= 0) {
      BOOST_THROW_EXCEPTION(std::runtime_error("Error reading from the input stream"));
    }

    auto data = std::make_shared<ndn::Data>(Name(m_dataPrefix).appendSegment(m_currentSegmentNo));

    if (m_insertStream->peek() == std::istream::traits_type::eof()) {
      data->setFinalBlock(ndn::name::Component::fromSegment(m_currentSegmentNo));
      m_isFinished = true;
    }

    data->setContent(buffer, readSize);
    data->setFreshnessPeriod(m_freshnessPeriod);
    putFileSignData(*data);

    m_data.insert(std::make_pair(m_currentSegmentNo, data));

    ++m_currentSegmentNo;
    delete[] buffer;
  }
}

void
DIFS::putFileSendManifest(const ndn::Name& prefix, const ndn::Interest& interest)
{
  BOOST_ASSERT(prefix == m_dataPrefix);

  if (prefix != interest.getName()) {
    if (m_verbose) {
      std::cerr << "Received unexpected interest " << interest << std::endl;
    }
    return;
  }

  ndn::Data data(interest.getName());
  auto blockCount = m_bytes / m_blockSize + (m_bytes % m_blockSize != 0);

  Manifest manifest(interest.getName().toUri(), 0, blockCount - 1);
  std::string json = manifest.toInfoJson();
  data.setContent((uint8_t*) json.data(), (size_t) json.size());
  data.setFreshnessPeriod(m_freshnessPeriod);
  putFileSignData(data);

  m_face.put(data);
}

void
DIFS::putFileSignData(ndn::Data& data)
{
  if (m_useDigestSha256) {
    m_keyChain.sign(data, ndn::signingWithSha256());
  }
  else if (m_identityForData.empty())
    m_keyChain.sign(data);
  else {
    m_keyChain.sign(data, ndn::signingByIdentity(m_identityForData));
  }
}

void
DIFS::putFileStopProcess()
{
  m_face.getIoService().stop();
}
}
