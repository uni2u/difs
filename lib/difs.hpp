#ifndef DIFS_HPP
#define DIFS_HPP

#include <iostream>
#include <string>
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

static const uint64_t DEFAULT_BLOCK_SIZE = 8600;
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

  DIFS(const std::string& common_name, int interestLifetime, int timeout, bool verbose)
  : m_common_name(common_name)
    , m_interestLifetime(interestLifetime)
    , m_timeout(timeout)
    , m_verbose(verbose)
    , m_validatorConfig(m_face)
    , m_scheduler(m_face.getIoService())
    , m_cmdSigner((ndn::KeyChain&)m_hcKeyChain)
  {
    parseConfig();
  }

  DIFS(ndn::Name repoPrefix)
  : m_repoPrefix(repoPrefix)
    , m_useDigestSha256(false)
    , m_freshnessPeriod(DEFAULT_FRESHNESS_PERIOD)
    , m_interestLifetime(DEFAULT_INTEREST_LIFETIME)
    , m_hasTimeout(false)
    , m_timeout(0)
    , m_blockSize(DEFAULT_BLOCK_SIZE)
    , m_insertStream(nullptr)
    , m_verbose(false)
    , m_processId(0)
    , m_checkPeriod(DEFAULT_CHECK_PERIOD)
    , m_currentSegmentNo(0)
    , m_validatorConfig(m_face)
    , m_scheduler(m_face.getIoService())
    , m_cmdSigner((ndn::KeyChain&)m_hcKeyChain)
  {}

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
  setIdentityForData(std::string identityForData);

  void
  setIdentityForCommand(std::string identityForCommand);

  void
  deleteFile(const ndn::Name& name);

  void
  deleteNode(const std::string from, const std::string to);

  void
  getFile(const ndn::Name& name, std::ostream& os);

  void
  putFile(const ndn::Name& name, std::istream& is, const std::string identityForData, const std::string identityForCommand);

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

  // ndn::Interest
  // generateCommandInterest(const ndn::Name& commandPrefix, const std::string& command,
  //   const repo::RepoCommandParameter& commandParameter);

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
  ndn::Name m_common_name;
  ndn::Name m_repoPrefix;
  bool m_useDigestSha256;
  ndn::time::milliseconds m_freshnessPeriod; 
  ndn::time::milliseconds m_interestLifetime;
  bool m_hasTimeout;
  ndn::time::milliseconds m_timeout;
  size_t m_blockSize;
  std::istream* m_insertStream;
  bool m_verbose;
  uint64_t m_processId;
  ndn::time::milliseconds m_checkPeriod;
  size_t m_currentSegmentNo;
  std::string m_identityForData;
  std::string m_identityForCommand;
  ndn::Name m_ndnName;

  ndn::Name m_dataPrefix;
  int m_retryCount;

  std::ostream* m_os;
  size_t m_bytes;
  boost::property_tree::ptree m_validatorNode;

  // repo::Manifest m_manifest;
  std::string m_manifest;

	std::map<int, const ndn::Block> map;
	int m_currentSegment, m_totalSize;

  ndn::time::milliseconds timeout;
  ndn::DelegationList m_forwardingHint, m_nodePrefix;
  ndn::HCKeyChain m_hcKeyChain;

  ndn::Face m_face;
  ndn::security::ValidatorConfig m_validatorConfig;
  ndn::Scheduler m_scheduler;
  using DataContainer = std::vector<shared_ptr<ndn::Data>>;
  DataContainer m_data;
  ndn::security::CommandInterestSigner m_cmdSigner;
};

}// namespace difs

#endif  // DIFS_HPP