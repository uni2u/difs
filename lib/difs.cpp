#include "repo-command-parameter.hpp"
#include "repo-command-response.hpp"
#include "util.hpp"

#include "manifest/manifest.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/command-interest-signer.hpp>
#include <ndn-cxx/security/hc-key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/logger.hpp>

#include "difs.hpp"

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <string>
#include <time.h>

#include <boost/asio.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/read.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/info_parser.hpp>

NDN_LOG_INIT(difs.DIFS);

static const size_t PRE_SIGN_DATA_COUNT = 11;

namespace difs {

using ndn::Block;
using ndn::Data;
using ndn::Interest;
using ndn::Name;

using std::bind;
using std::placeholders::_1;
using std::placeholders::_2;

using namespace repo;

static const int MAX_RETRY = 3;
int m_totalSize = 0;

// Get SegmentFetcher validation
void
DIFS::parseConfig() {
  std::string configPath = "node.conf";

  std::ifstream fin(configPath.c_str());
  if (!fin.is_open())
    std::cout << "open error" << std::endl;

  using namespace boost::property_tree;
  ptree propertyTree;
  try {
    read_info(fin, propertyTree);
  }
  catch (const ptree_error& e) {
    std::cout << "failed to read configuration file '" + configPath + "'" << std::endl;
  }

  ptree repoConf = propertyTree.get_child("repo");

  m_validatorNode = repoConf.get_child("validator");
  m_validatorConfig.load(m_validatorNode, configPath);
}

void
DIFS::run()
{
  m_face.processEvents();
}

// Set
void
DIFS::setNodePrefix(ndn::DelegationList nodePrefix)
{
  m_nodePrefix = nodePrefix;
}
void
DIFS::setForwardingHint(ndn::DelegationList forwardingHint)
{
  m_forwardingHint = forwardingHint;
}
void
DIFS::setRepoPrefix(Name repoPrefix)
{
  m_repoPrefix = repoPrefix;
}
void
DIFS::setTimeOut(ndn::time::milliseconds timeout)
{
  m_timeout = timeout;
}
void
DIFS::setInterestLifetime(ndn::time::milliseconds interestLifetime)
{
  m_interestLifetime = interestLifetime;
}
void
DIFS::setFreshnessPeriod(ndn::time::milliseconds freshnessPeriod)
{
  m_freshnessPeriod = freshnessPeriod;
}
void
DIFS::setHasTimeout(bool hasTimeout)
{
  m_hasTimeout = hasTimeout;
}
void
DIFS::setVerbose(bool verbose)
{
  m_verbose = verbose;
}
void
DIFS::setUseDigestSha256(bool useDigestSha256)
{
  m_useDigestSha256 = useDigestSha256;
}
void
DIFS::setUseHashChain(bool useHashChain) 
{
  m_useHashChain = useHashChain;
}

void
DIFS::setBlockSize(size_t blockSize)
{
  m_blockSize = blockSize;
}
void
DIFS::setIdentityForData(std::string identityForData) 
{
  m_identityForData = identityForData;
}

void
DIFS::setIdentityForCommand(std::string identityForCommand)
{
  m_identityForCommand = identityForCommand;
}

// Delete Node
void
DIFS::deleteNode(const std::string from, const std::string to)
{
  RepoCommandParameter parameter;
  parameter.setFrom(ndn::encoding::makeBinaryBlock(tlv::From, from.c_str(), from.length()));
  parameter.setTo(ndn::encoding::makeBinaryBlock(tlv::To, to.c_str(), to.length()));

  Name cmd = m_repoPrefix;
  cmd.append("del-node").append(parameter.wireEncode());

  ndn::Interest deleteNodeInterest = m_cmdSigner.makeCommandInterest(cmd);

  if (!m_forwardingHint.empty())
    deleteNodeInterest.setForwardingHint(m_forwardingHint);

  deleteNodeInterest.setInterestLifetime(m_interestLifetime);
  deleteNodeInterest.setMustBeFresh(true);

  m_face.expressInterest(deleteNodeInterest, std::bind(&DIFS::onDeleteNodeCommandResponse, this, _1, _2),
                          std::bind(&DIFS::onDeleteNodeCommandNack, this, _1), // Nack
                          std::bind(&DIFS::onDeleteNodeCommandTimeout, this, _1));
}

void
DIFS::onDeleteNodeCommandResponse(const ndn::Interest& interest, const ndn::Data& data)
{
  std::cout << "Delete Node" << std::endl;
}

void
DIFS::onDeleteNodeCommandTimeout(const Interest& interest)
{
  std::cout << "Delete Node Timeout" << std::endl;
}

void
DIFS::onDeleteNodeCommandNack(const Interest& interest)
{
  std::cout << "NACK: Delete Node" << std::endl;
}

// Delete
void
DIFS::deleteFile(const Name& name)
{
  RepoCommandParameter parameter;
  parameter.setProcessId(0); // FIXME: set process id properly
  parameter.setName(name);

  Name cmd = m_repoPrefix;
  cmd.append("delete").append(parameter.wireEncode());

  ndn::Interest commandInterest = m_cmdSigner.makeCommandInterest(cmd);
  commandInterest.setInterestLifetime(m_interestLifetime);
  commandInterest.setMustBeFresh(true);

  if (!m_forwardingHint.empty())
    commandInterest.setForwardingHint(m_forwardingHint);

  m_face.expressInterest(commandInterest, std::bind(&DIFS::onDeleteCommandResponse, this, _1, _2),
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
  }
  else {
    std::cerr << "TIMEOUT: last interest sent" << std::endl << "TIMEOUT: abort fetching after " << MAX_RETRY << " times of retry" << std::endl;
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
  }
  else {
    std::cerr << "NACK: last interest sent" << std::endl << "NACK: abort fetching after " << MAX_RETRY << " times of retry" << std::endl;
  }
}

void
DIFS::getInfo()
{
  Name cmd = m_repoPrefix;
  cmd.append("nodeinfo").appendSegment(0);
  ndn::Interest commandInterest(cmd);
  commandInterest.setInterestLifetime(m_interestLifetime);
  commandInterest.setMustBeFresh(true);

  if (!m_forwardingHint.empty()) {
    commandInterest.setForwardingHint(m_forwardingHint);
  }

  m_face.expressInterest(commandInterest, std::bind(&DIFS::onGetInfoCommandResponse, this, _1, _2),
                          std::bind(&DIFS::onGetInfoCommandNack, this, _1), // Nack
                          std::bind(&DIFS::onGetInfoCommandTimeout, this, _1));
}

void
DIFS::onGetInfoCommandResponse(const ndn::Interest& interest, const ndn::Data& data)
{
  auto content = data.getContent();
  std::string msg = reinterpret_cast<const char*>(content.value());
  msg = msg.substr(0, content.value_size());
  std::cout << msg;

  if (data.getFinalBlock().value().toSegment() > 0) {
    infoFetch(1);
  }
}

void
DIFS::onGetInfoCommandTimeout(const Interest& interest)
{
  if (m_retryCount++ < MAX_RETRY) {
    getInfo();

    if (m_verbose) {
      std::cerr << "TIMEOUT: retransmit interest for " << interest.getName() << std::endl;
    }
  }
  else {
    std::cerr << "TIMEOUT: last interest sent" << std::endl << "TIMEOUT: abort fetching after " << MAX_RETRY << " times of retry" << std::endl;
  }
}

void
DIFS::onGetInfoCommandNack(const Interest& interest)
{
  if (m_retryCount++ < MAX_RETRY) {
    getInfo();

    if (m_verbose) {
      std::cerr << "NACK: retransmit interest for " << interest.getName() << std::endl;
    }
  }
  else {
    std::cerr << "NACK: last interest sent" << std::endl << "NACK: abort fetching after " << MAX_RETRY << " times of retry" << std::endl;
  }
}

void
DIFS::infoFetch(int start)
{
  ndn::Interest interest(m_repoPrefix.append("nodeinfo").appendSegment(start));
  boost::chrono::milliseconds lifeTime(m_interestLifetime);
  interest.setInterestLifetime(lifeTime);
  interest.setMustBeFresh(false);

  ndn::security::Validator& m_validator(m_validatorConfig);

  ndn::util::SegmentFetcher::Options options;
  options.useConstantCwnd = true; //set windowsize
  options.initCwnd = 12;
  options.useConstantInterestTimeout = true; //set interest lifetime
  options.interestLifetime = lifeTime;
  options.maxTimeout = lifeTime;

  std::shared_ptr<ndn::util::SegmentFetcher> fetcher;
  auto Fetcher = fetcher->start(m_face, interest, m_validator, options);
  Fetcher->onError.connect([](uint32_t errorCode, const std::string& errorMsg) { std::cout << "Error: " << errorMsg << std::endl; });
  Fetcher->afterSegmentValidated.connect([this, Fetcher](const Data& data) { onGetInfoDataCommandResponse(data); });
  Fetcher->afterSegmentTimedOut.connect([this, Fetcher]() { onGetInfoDataCommandTimeout(*Fetcher); });
}

void
DIFS::onGetInfoDataCommandResponse(const ndn::Data& data)
{
  auto content = data.getContent();
  std::string msg = reinterpret_cast<const char*>(content.value());
  msg = msg.substr(0, content.value_size());
  std::cout << msg;
}

void
DIFS::onGetInfoDataCommandTimeout(ndn::util::SegmentFetcher& fetcher)
{
  NDN_LOG_INFO("INFO: onGetInfoDataCommandTimeout = timeout");
//  std::cout << "timeout" << std::endl;
}

bool
DIFS::getUseHashChain() {
  return m_useHashChain;
}

bool
DIFS::getUseDigestSha256() {
  return m_useDigestSha256;
}

void
DIFS::getKeySpaceInfo()
{
  Name cmd = m_repoPrefix;
  cmd.append("ringinfo");

  ndn::Interest commandInterest(cmd);
  commandInterest.setInterestLifetime(m_interestLifetime);
  commandInterest.setMustBeFresh(true);

  if (!m_forwardingHint.empty()) {
    commandInterest.setForwardingHint(m_forwardingHint);
  }

  m_face.expressInterest(commandInterest, std::bind(&DIFS::onGetKeySpaceInfoCommandResponse, this, _1, _2),
                          std::bind(&DIFS::onGetKeySpaceInfoCommandNack, this, _1), // Nack
                          std::bind(&DIFS::onGetKeySpaceInfoCommandTimeout, this, _1));
}

void
DIFS::onGetKeySpaceInfoCommandResponse(const ndn::Interest& interest, const ndn::Data& data)
{
  std::cout << data.getContent().value();
}

void
DIFS::onGetKeySpaceInfoCommandTimeout(const Interest& interest)
{
  if (m_retryCount++ < MAX_RETRY) {
    getKeySpaceInfo();

    if (m_verbose) {
      std::cerr << "TIMEOUT: retransmit interest for " << interest.getName() << std::endl;
    }
  }
  else {
    std::cerr << "TIMEOUT: last interest sent" << std::endl << "TIMEOUT: abort fetching after " << MAX_RETRY << " times of retry" << std::endl;
  }
}

void
DIFS::onGetKeySpaceInfoCommandNack(const Interest& interest)
{
  if (m_retryCount++ < MAX_RETRY) {
    getKeySpaceInfo();

    if (m_verbose) {
      std::cerr << "NACK: retransmit interest for " << interest.getName() << std::endl;
    }
  }
  else {
    std::cerr << "NACK: last interest sent" << std::endl << "NACK: abort fetching after " << MAX_RETRY << " times of retry" << std::endl;
  }
}

// Get
void
DIFS::getFile(const Name& dataName, std::ostream& os)
{
  RepoCommandParameter parameter;
  parameter.setName(dataName);

  m_os = &os;
  Name cmd = Name("/get").append(parameter.wireEncode());

  ndn::Interest commandInterest = m_cmdSigner.makeCommandInterest(cmd);
  commandInterest.setCanBePrefix(true);
  commandInterest.setMustBeFresh(false);
  commandInterest.setInterestLifetime(m_interestLifetime);

  if (!m_forwardingHint.empty()) {
    commandInterest.setForwardingHint(m_forwardingHint);
  }

  m_face.expressInterest(commandInterest, std::bind(&DIFS::onGetCommandResponse, this, _1, _2),
                          std::bind(&DIFS::onGetCommandNack, this, _1),
                          std::bind(&DIFS::onGetCommandTimeout, this, _1));
}

void
DIFS::onGetCommandResponse(const Interest& interest, const Data& data)
{
  auto content = data.getContent();
  std::string json(content.value_begin(), content.value_end());

  if (json.length() == 0) {
    std::cerr << "Not found" << std::endl;
    return;
  }

  m_manifest = json;
  fetch(0);
}

void
DIFS::onGetCommandTimeout(const Interest& interest)
{
  if (m_retryCount++ < MAX_RETRY) {
    getFile(interest.getName(), *m_os);

    if (m_verbose) {
      std::cerr << "TIMEOUT: retransmit interest for " << interest.getName() << std::endl;
    }
  }
  else {
    std::cerr << "TIMEOUT: last interest sent" << std::endl << "TIMEOUT: abort fetching after " << MAX_RETRY << " times of retry" << std::endl;
  }
}

void
DIFS::onGetCommandNack(const Interest& interest)
{
  if (m_retryCount++ < MAX_RETRY) {
    getFile(interest.getName(), *m_os);

    if (m_verbose) {
      std::cerr << "NACK: retransmit interest for " << interest.getName() << std::endl;
    }
  }
  else {
    std::cerr << "NACK: last interest sent" << std::endl << "NACK: abort fetching after " << MAX_RETRY << " times of retry" << std::endl;
  }
}

void
DIFS::fetch(int start)
{
  auto manifest = Manifest::fromJson(m_manifest);
  auto repos = manifest.getRepos();

  ndn::Interest interest(Name(manifest.getName()).appendSegment(0));
  boost::chrono::milliseconds lifeTime(m_interestLifetime);
  interest.setInterestLifetime(lifeTime);
  interest.setMustBeFresh(false);

  ndn::Delegation d;
  d.name = Name(repos.begin()->name);
  interest.setForwardingHint(ndn::DelegationList{ d });

  ndn::security::Validator& m_validator(m_validatorConfig);

  ndn::util::SegmentFetcher::Options options;
  options.useConstantCwnd = true; //set windowsize
  options.initCwnd = 12;
  options.useConstantInterestTimeout = true; //set interest lifetime
  options.interestLifetime = lifeTime;
  options.maxTimeout = lifeTime;

  std::shared_ptr<ndn::util::HCSegmentFetcher> hc_fetcher;
  auto hcFetcher = hc_fetcher->start(m_face, interest, m_validator, options);
  hcFetcher->onError.connect([](uint32_t errorCode, const std::string& errorMsg) { std::cout << "Error: " << errorMsg << std::endl; });
  hcFetcher->afterSegmentValidated.connect([this, hcFetcher](const Data& data) { onDataCommandResponse(data); });
  hcFetcher->afterSegmentTimedOut.connect([this, hcFetcher]() { onDataCommandTimeout(*hcFetcher); });
}

void
DIFS::onDataCommandResponse(const ndn::Data& data)
{
  const auto& content = data.getContent();
  m_os->write(reinterpret_cast<const char*>(content.value()), content.value_size());

  int segment = data.getName().get(-1).toSegment();
  int finalBlockId = data.getFinalBlock().value().toSegment();
  m_totalSize += content.value_size();

  NDN_LOG_INFO("INFO: data name = " << data.getName() << ", next-hash = " << data.getMetaInfo().getAppMetaInfo().front());

  if (segment == finalBlockId) {
    NDN_LOG_INFO("INFO: Total # bytes of content received: " << m_totalSize);
    m_totalSize = 0;
  }
}

void
DIFS::onDataCommandTimeout(ndn::util::HCSegmentFetcher& fetcher)
{
  NDN_LOG_INFO("INFO: onDataCommandTimeout = timeout");
//  std::cout << "Timeout" << std::endl;
}

void
DIFS::putFile(const ndn::Name& ndnName, std::istream& is, const std::string identityForData,  const std::string IdentityForCommand)
{
  setIdentityForData(identityForData);
  setIdentityForCommand(IdentityForCommand);
  m_dataPrefix = ndnName;
  //m_repoPrefix = ndnName;

  m_insertStream = &is;
  m_insertStream->seekg(0, std::ios::beg);
  auto beginPos = m_insertStream->tellg();
  m_insertStream->seekg(0, std::ios::end);
  m_bytes = m_insertStream->tellg() - beginPos;
  m_insertStream->seekg(0, std::ios::beg);

  putFilePrepareNextData();

  std::cout<<"m_dataPrefix"<<m_dataPrefix<<std::endl;
  m_face.setInterestFilter(m_dataPrefix,
                           bind(&DIFS::onPutFileInterest, this, _1, _2),
                           bind(&DIFS::onPutFileRegisterSuccess, this, _1),
                           bind(&DIFS::onPutFileRegisterFailed, this, _1, _2));
  std::cout<<"m_face.setInterestFilter"<<std::endl;

  if (m_hasTimeout)
    m_scheduler.schedule(m_timeout, [this] { putFileStopProcess(); });
}

void
DIFS::onPutFileInterest(const Name& prefix, const ndn::Interest& interest)
{
  if (interest.getName().size() == prefix.size()) {
    putFileSendManifest(prefix, interest);
    return;
  }

  uint64_t segmentNo;
  try {
    NDN_LOG_DEBUG("segmentNo" <<segmentNo<< std::endl);
    Name::Component segmentComponent = interest.getName().get(prefix.size());
    NDN_LOG_DEBUG("segmentComponent" <<segmentComponent.toUri()<< std::endl);
    segmentNo = segmentComponent.toSegment();
    NDN_LOG_DEBUG("--segmentNo" <<segmentNo<< std::endl);
  }
  catch (const tlv::Error& e) {
    NDN_LOG_DEBUG("tlv::Error failed" << std::endl);

    if (m_verbose) {
      NDN_LOG_ERROR("Error processing incoming interest " << interest << ": " << e.what());
    }
    return;
  }

  shared_ptr<Data> data;

  if (segmentNo < m_data.size()) {
    data = m_data[segmentNo];
  }
  else if (interest.matchesData(*m_data[0])) {
    data = m_data[0];
  }

  if (data != nullptr) {
    m_face.put(*data);
  }
  else {
    m_face.put(ndn::lp::Nack(interest));
  }
}

void
DIFS::onPutFileRegisterSuccess(const Name& prefix)
{
  putFileStartInsertCommand();
}

void
DIFS::onPutFileRegisterFailed(const Name& prefix, const std::string& reason)
{
  BOOST_THROW_EXCEPTION(std::runtime_error("onRegisterFailed: " + reason));
}

void
DIFS::putFileStartInsertCommand()
{
  RepoCommandParameter parameters;
  parameters.setName(m_dataPrefix);

  if (!m_nodePrefix.empty())
    parameters.setNodePrefix(m_nodePrefix);

  Name cmd = m_repoPrefix;
  cmd.append("insert").append(parameters.wireEncode());

  ndn::Interest commandInterest;

  if (m_identityForCommand.empty())
    commandInterest = m_cmdSigner.makeCommandInterest(cmd);
  else {
    commandInterest = m_cmdSigner.makeCommandInterest(cmd, ndn::signingByIdentity(m_identityForCommand));
  }

  if (!m_forwardingHint.empty())
    commandInterest.setForwardingHint(m_forwardingHint);

  commandInterest.setInterestLifetime(m_interestLifetime);

  m_face.expressInterest(commandInterest, bind(&DIFS::onPutFileInsertCommandResponse, this, _1, _2),
                          bind(&DIFS::onPutFileInsertCommandTimeout, this, _1), // Nack
                          bind(&DIFS::onPutFileInsertCommandNack, this, _1));
}

void
DIFS::onPutFileInsertCommandResponse(const ndn::Interest& interest, const ndn::Data& data)
{
  RepoCommandResponse response(data.getContent().blockFromValue());
  auto statusCode = response.getCode();

  if (statusCode >= 400) {
    BOOST_THROW_EXCEPTION(std::runtime_error("insert command failed with code: " + boost::lexical_cast<std::string>(statusCode)));
  }
  m_processId = response.getProcessId();

  m_scheduler.schedule(m_checkPeriod, [this] { putFileStartCheckCommand(); });
}

void
DIFS::onPutFileInsertCommandTimeout(const ndn::Interest& interest)
{
  if (m_retryCount++ < MAX_RETRY) {
    putFileStartInsertCommand();

    if (m_verbose) {
      std::cerr << "TIMEOUT: retransmit interest for " << interest.getName() << std::endl;
    }
  }
  else {
    std::cerr << "TIMEOUT: last interest sent" << std::endl << "TIMEOUT: abort fetching after " << MAX_RETRY << " times of retry" << std::endl;
  }
}

void
DIFS::onPutFileInsertCommandNack(const ndn::Interest& interest)
{
  if (m_retryCount++ < MAX_RETRY) {
    putFileStartInsertCommand();

    if (m_verbose) {
      std::cerr << "NACK: retransmit interest for " << interest.getName() << std::endl;
    }
  }
  else {
    std::cerr << "NACK: last interest sent" << std::endl << "NACK: abort fetching after " << MAX_RETRY << " times of retry" << std::endl;
  }
}

void
DIFS::putFileStartCheckCommand()
{
  RepoCommandParameter parameter;

  Name cmd = m_repoPrefix;
  cmd.append("insert check").append(parameter.setProcessId(m_processId).wireEncode());

  ndn::Interest checkInterest;

  if (m_identityForCommand.empty())
    checkInterest = m_cmdSigner.makeCommandInterest(cmd);
  else {
    checkInterest = m_cmdSigner.makeCommandInterest(cmd, ndn::signingByIdentity(m_identityForCommand));
  }

  if (!m_forwardingHint.empty())
    checkInterest.setForwardingHint(m_forwardingHint);

  checkInterest.setInterestLifetime(m_interestLifetime);

  m_face.expressInterest(checkInterest, bind(&DIFS::onPutFileCheckCommandResponse, this, _1, _2),
                          bind(&DIFS::onPutFileCheckCommandTimeout, this, _1), // Nack
                          bind(&DIFS::onPutFileCheckCommandTimeout, this, _1));
}

void
DIFS::onPutFileCheckCommandResponse(const ndn::Interest& interest, const ndn::Data& data)
{
  RepoCommandResponse response(data.getContent().blockFromValue());
  auto statusCode = response.getCode();

  if (statusCode >= 400) {
    BOOST_THROW_EXCEPTION(std::runtime_error("Insert check command failed with code: " + boost::lexical_cast<std::string>(statusCode)));
  }

  uint64_t insertCount = response.getInsertNum();

  if (insertCount == m_currentSegmentNo) {
    m_face.getIoService().stop();
    return;
  }

  m_scheduler.schedule(m_checkPeriod, [this] { putFileStartCheckCommand(); });
}

void
DIFS::onPutFileCheckCommandTimeout(const ndn::Interest& interest)
{
  if (m_retryCount++ < MAX_RETRY) {
    putFileStartCheckCommand();

    if (m_verbose) {
      std::cerr << "TIMEOUT: retransmit interest for " << interest.getName() << std::endl;
    }
  }
  else {
    std::cerr << "TIMEOUT: last interest sent" << std::endl << "TIMEOUT: abort fetching after " << MAX_RETRY << " times of retry" << std::endl;
  }
}

void
DIFS::onPutFileCheckCommandNack(const ndn::Interest& interest)
{
  if (m_retryCount++ < MAX_RETRY) {
    putFileStartCheckCommand();

    if (m_verbose) {
      std::cerr << "NACK: retransmit interest for " << interest.getName() << std::endl;
    }
  }
  else {
    std::cerr << "NACK: last interest sent" << std::endl << "NACK: abort fetching after " << MAX_RETRY << " times of retry" << std::endl;
  }
}

void
DIFS::putFilePrepareNextData()
{
  int chunkSize = m_bytes / m_blockSize;
  auto finalBlockId = ndn::name::Component::fromSegment(chunkSize);

  for (int count = 0; count <= chunkSize; count++) {
    uint8_t* buffer = new uint8_t[m_blockSize];
    m_insertStream->read(reinterpret_cast<char*>(buffer), m_blockSize);

    auto readSize = m_insertStream->gcount();

    if (readSize > 0) {
      auto data = std::make_shared<ndn::Data>(Name(m_dataPrefix).appendSegment(count));
      Block content = ndn::encoding::makeBinaryBlock(tlv::Content, buffer, readSize);
      data->setFreshnessPeriod(m_freshnessPeriod);
      data->setContent(content);
      data->setFinalBlock(finalBlockId);

      m_data.push_back(data);
    }
    else {
      m_data.clear();
      return;
    }
  }
  uint8_t zeros[32] = {0,0,0,0,0,0,0,0,0,0,
                       0,0,0,0,0,0,0,0,0,0,
                       0,0,0,0,0,0,0,0,0,0,
                       0,0};
  uint8_t* zerop = zeros;

  Block nextHash = ndn::encoding::makeBinaryBlock(tlv::NextHashValue, zerop, 32);
  //Block nextHash(ndn::lp::tlv::HashChain);

  for (auto iter = m_data.rbegin(); iter != m_data.rend(); iter++) {
    Name tmp = Name(m_identityForData);
    if (iter == m_data.rend() - 1) {
      if(getUseHashChain()) {
        NDN_LOG_INFO("hc on signingByHashChainIdentity\n");
      m_hcKeyChain.sign(**iter, nextHash, ndn::signingByHashChainIdentity(tmp));
      } else {
        NDN_LOG_INFO("hc off\n");
        m_hcKeyChain.sign(**iter, ndn::signingByIdentity(m_identityForData));
      }
    }
    else {
      if(getUseHashChain()) {
        NDN_LOG_INFO("hc on signerTypeHashChainSha256\n");
        m_hcKeyChain.sign(**iter, nextHash, ndn::security::SigningInfo(ndn::security::SigningInfo::SIGNER_TYPE_HASHCHAIN_SHA256));
      } else if(getUseDigestSha256()){
        NDN_LOG_INFO("hc off and sha256\n");
        m_hcKeyChain.sign(**iter, ndn::signingWithSha256());
      } else {
        NDN_LOG_INFO("hc off and identity\n");
        m_hcKeyChain.sign(**iter, ndn::signingByIdentity(m_identityForData));
      }
    }

    if(getUseHashChain()) {
      nextHash = ndn::encoding::makeBinaryBlock(tlv::NextHashValue, (*iter)->getSignatureValue().value(), (*iter)->getSignatureValue().value_size());
    }
    //nextHash = ndn::encoding::makeBinaryBlock(ndn::lp::tlv::HashChain, (*iter)->getSignatureValue().value(), (*iter)->getSignatureValue().value_size());
    m_currentSegmentNo++;
  }
}

void
DIFS::putFileSendManifest(const Name& prefix, const ndn::Interest& interest)
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
  data.setContent((uint8_t*)json.data(), (size_t)json.size());
  data.setFreshnessPeriod(3_s);
  m_hcKeyChain.ndn::KeyChain::sign(data);

  m_face.put(data);
}

void
DIFS::putFileStopProcess()
{
  m_face.getIoService().stop();
}

} // namespace difs

extern "C" {
difs::DIFS* DIFS_new() { return new difs::DIFS(); }
void DIFS_parseConfig(difs::DIFS* self) { self->parseConfig(); }
void DIFS_setNodePrefix(difs::DIFS* self, ndn::DelegationList nodePrefix) { self->setNodePrefix(nodePrefix); }
void DIFS_setForwardingHint(difs::DIFS* self, ndn::DelegationList forwardingHint) { self->setForwardingHint(forwardingHint); }
void DIFS_setRepoPrefix(difs::DIFS* self, ndn::Name repoPrefix) { self->setRepoPrefix(repoPrefix); }
void DIFS_setTimeOut(difs::DIFS* self, ndn::time::milliseconds timeout) { self->setTimeOut(timeout); }
void DIFS_setInterestLifetime(difs::DIFS* self, ndn::time::milliseconds interestLifetime) { self->setInterestLifetime(interestLifetime); }
void DIFS_setFreshnessPeriod(difs::DIFS* self, ndn::time::milliseconds freshnessPeriod) { self->setFreshnessPeriod(freshnessPeriod); }
void DIFS_setHasTimeout(difs::DIFS* self, bool hasTimeout) { self->setHasTimeout(hasTimeout); }
void DIFS_setVerbose(difs::DIFS* self, bool verbose) { self->setVerbose(verbose); }
void DIFS_setUseDigestSha256(difs::DIFS* self, bool useDigestSha256) { self->setUseDigestSha256(useDigestSha256); }
void DIFS_setUseHashChain(difs::DIFS* self, bool useHashChain) { self->setUseHashChain(useHashChain); }
void DIFS_setBlockSize(difs::DIFS* self, size_t blockSize) { self->setBlockSize(blockSize); }
void DIFS_setIdentityForCommand(difs::DIFS* self, std::string identityForCommand) { self->setIdentityForCommand(identityForCommand); }
void DIFS_deleteFile(difs::DIFS* self, const ndn::Name& name) { self->deleteFile(name); }
void DIFS_deleteNode(difs::DIFS* self, const std::string from, const std::string to) { self->deleteNode(from, to); }
void DIFS_getFile(difs::DIFS* self, const ndn::Name& dataName, std::ostream& os) { self->getFile(dataName, os); }
void DIFS_putFile(difs::DIFS* self, const std::string dataPrefix, std::istream& is, std::string identityForData, std::string IdentityForCommand) { self->putFile(dataPrefix, is, identityForData,  IdentityForCommand); }
void DIFS_getInfo(difs::DIFS* self) { self->getInfo(); }
void DIFS_getKeySpaceInfo(difs::DIFS* self) { self->getKeySpaceInfo(); }
void DIFS_run(difs::DIFS* self) { self->run(); }
}

