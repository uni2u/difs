#ifndef DIFS_HPP
#define DIFS_HPP

#include <iostream>
#include <ndn-cxx/security/hc-key-chain.hpp>
#include <ndn-cxx/security/command-interest-signer.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/security/validator.hpp>
#include <ndn-cxx/security/validator-config.hpp>
#include <ndn-cxx/util/segment-fetcher.hpp>
#include <ndn-cxx/util/hc-segment-fetcher.hpp>

namespace difs {

using std::shared_ptr;

static const uint64_t DEFAULT_BLOCK_SIZE = 8500;
static const uint64_t DEFAULT_INTEREST_LIFETIME = 4000;
static const uint64_t DEFAULT_FRESHNESS_PERIOD = 10000;
static const uint64_t DEFAULT_CHECK_PERIOD = 1000;

class DIFS : boost::noncopyable
{
public:
  DIFS()
  : m_validatorConfig(m_face)
    , m_scheduler(m_face.getIoService())
    , m_cmdSigner(m_hcKeyChain)
  {}

  DIFS(const std::string& repoPrefix)
  : m_repoPrefix(repoPrefix)
    , m_freshnessPeriod(DEFAULT_FRESHNESS_PERIOD)
    , m_interestLifetime(DEFAULT_INTEREST_LIFETIME)
    , m_timeout(0)
    , m_checkPeriod(DEFAULT_CHECK_PERIOD)
    , m_useDigestSha256(false)
    , m_hasTimeout(false)
    , m_verbose(false)
    , m_blockSize(DEFAULT_BLOCK_SIZE)
    , m_insertStream(nullptr)
    , m_processId(0)
    , m_currentSegmentNo(0)
    , m_validatorConfig(m_face)
    , m_scheduler(m_face.getIoService())
    , m_cmdSigner((ndn::KeyChain&)m_hcKeyChain)
  {
    parseConfig();
  }

  void
  parseConfig();

  void
  setNodePrefix(ndn::DelegationList nodePrefix);

  void
  setForwardingHint(ndn::DelegationList forwardingHint);

  void
  setRepoPrefix(ndn::Name repoPrefix);

  void
  setTimeOut(ndn::time::milliseconds timeout);

  void
  setInterestLifetime(ndn::time::milliseconds interestLifetime);

  void
  setFreshnessPeriod(ndn::time::milliseconds freshnessPeriod);

  void
  setHasTimeout(bool hasTimeout);

  void
  setVerbose(bool verbose);

  void
  setUseDigestSha256(bool useDigestSha256);

  void
  setBlockSize(size_t blockSize);

  void
  setIdentityForCommand(std::string identityForCommand);

  void
  deleteFile(const ndn::Name& name);

  void
  deleteNode(const std::string from, const std::string to);

  void
  getFile(const ndn::Name& dataName, std::ostream& os);

  void
  putFile(const std::string dataPrefix, std::istream& is);

  void
  getInfo();

  void
  getKeySpaceInfo();

  void
  run();

private:
  void 
  fetch(int start);

  void 
  onDataCommandResponse(const ndn::Data& data);

  void 
  onDataCommandTimeout(ndn::util::HCSegmentFetcher& fetcher);

  void
  infoFetch(int start);

  void
  onGetInfoDataCommandResponse(const ndn::Data &data);

  void
  onGetInfoDataCommandTimeout(ndn::util::SegmentFetcher &fetcher);

  void
  onDeleteCommandTimeout(const ndn::Interest& interest);

  void
  onDeleteCommandNack(const ndn::Interest& interest);

  void
  onDeleteCommandResponse(const ndn::Interest& interest, const ndn::Data& data);

  void
  onDeleteNodeCommandTimeout(const ndn::Interest& interest);

  void
  onDeleteNodeCommandNack(const ndn::Interest& interest);

  void
  onDeleteNodeCommandResponse(const ndn::Interest& interest, const ndn::Data& data);

  void
  onGetCommandResponse(const ndn::Interest& interest, const ndn::Data& data);

  void
  onGetCommandNack(const ndn::Interest& interest);

  void
  onGetCommandTimeout(const ndn::Interest& interest);

  void
  onRegisterSuccess(const ndn::Name& prefix);

  void
  onRegisterFailed(const ndn::Name& prefix, const std::string& reason);

  void
  putFileSendManifest(const ndn::Name &prefix, const ndn::Interest &interest);

  void
  onPutFileInterest(const ndn::Name& prefix, const ndn::Interest& interest);

  void
  onPutFileInsertCommandResponse(const ndn::Interest& interest, const ndn::Data& data);
 
  void
  onPutFileInsertCommandTimeout(const ndn::Interest& interest);
 
  void
  onPutFileRegisterSuccess(const ndn::Name& prefix);

  void
  onPutFileRegisterFailed(const ndn::Name& prefix, const std::string& reason);
  
  void
  onPutCommandNack(const ndn::Interest& interest);

  void
  onPutCommandTimeout(const ndn::Interest& interest);

  void
  onPutFileCheckCommandNack(const ndn::Interest& interest);

  void
  onPutFileInsertCommandNack(const ndn::Interest& interest);
  
  void
  putFileStopProcess();

  void
  onPutFileCheckCommandResponse(const ndn::Interest& interest, const ndn::Data& data);
  
  void
  onPutFileCheckCommandTimeout(const ndn::Interest& interest);

  void
  putFilePrepareNextData();

  void
  putFileStartCheckCommand();

  void
  putFileonCheckCommandResponse(const ndn::Interest& interest, const ndn::Data& data);

  void
  putFileStartInsertCommand();

  void
  onGetInfoCommandTimeout(const ndn::Interest& interest);

  void
  onGetInfoCommandNack(const ndn::Interest& interest);

  void
  onGetInfoCommandResponse(const ndn::Interest& interest, const ndn::Data& data);

  void
  onGetKeySpaceInfoCommandTimeout(const ndn::Interest& interest);

  void
  onGetKeySpaceInfoCommandNack(const ndn::Interest& interest);

  void
  onGetKeySpaceInfoCommandResponse(const ndn::Interest& interest, const ndn::Data& data);

private:
  ndn::Name m_repoPrefix, m_dataPrefix;
  ndn::time::milliseconds m_freshnessPeriod, m_interestLifetime, m_timeout, m_checkPeriod;  
  bool m_useDigestSha256, m_hasTimeout, m_verbose;
  std::istream* m_insertStream;
  uint64_t m_processId;
  size_t m_currentSegmentNo;

  std::ostream* m_os;
  size_t m_blockSize, m_bytes;
  boost::property_tree::ptree m_validatorNode;

  std::string m_identityForCommand, m_manifest;

  std::map<int, const ndn::Block> map;
  int m_retryCount, m_currentSegment, m_totalSize;

  ndn::DelegationList m_forwardingHint, m_nodePrefix;
  ndn::HCKeyChain m_hcKeyChain;

  ndn::Face m_face;
  ndn::security::ValidatorConfig m_validatorConfig;
  ndn::Scheduler m_scheduler;
  std::vector<shared_ptr<ndn::Data>> m_data;
  ndn::security::CommandInterestSigner m_cmdSigner;
};

}// namespace difs

#endif  // DIFS_HPP
