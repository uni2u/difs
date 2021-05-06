#ifndef DIFS_HPP
#define DIFS_HPP

#include <iostream>
#include <ndn-cxx/security/command-interest-signer.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/scheduler.hpp>

namespace difs {

using std::shared_ptr;

static const uint64_t DEFAULT_BLOCK_SIZE = 1000;
static const uint64_t DEFAULT_INTEREST_LIFETIME = 4000;
static const uint64_t DEFAULT_FRESHNESS_PERIOD = 10000;
static const uint64_t DEFAULT_CHECK_PERIOD = 1000;

class DIFS : boost::noncopyable
{
public:
  DIFS()
  : m_scheduler(m_face.getIoService())
    , m_cmdSigner(m_keyChain)
  {}

  DIFS(const std::string& common_name, int interestLifetime, int timeout, bool verbose)
  : m_common_name(common_name)
    , m_interestLifetime(interestLifetime)
    , m_timeout(timeout)
    , m_verbose(verbose)
    , m_scheduler(m_face.getIoService())
    , m_cmdSigner(m_keyChain)
  {}

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
    , m_isFinished(false)
    , m_scheduler(m_face.getIoService())
    , m_cmdSigner(m_keyChain)
  {}

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
  getFile(const ndn::Name& name, std::ostream& os);

  void
  putFile(const ndn::Name& name, std::istream& is);

  void
  run();

private:
  void
  onDeleteCommandTimeout(const ndn::Interest& interest);

  void
  onDeleteCommandNack(const ndn::Interest& interest);

  void
  onDeleteCommandResponse(const ndn::Interest& interest, const ndn::Data& data);

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
  putFileSignData(ndn::Data& data);

  void
  onPutFileCheckCommandResponse(const ndn::Interest& interest, const ndn::Data& data);
  
  void
  onPutFileCheckCommandTimeout(const ndn::Interest& interest);
  
  // ndn::Interest
  // putFileGenerateCommandInterest(const ndn::Name& commandPrefix, const std::string& command,
  //                         const repo::RepoCommandParameter& commandParameter);

  // void
  // createManifestData(const ndn::Name& prefix, const ndn::Interest& interest);

  // void
  // getFileFetchData(const repo::Manifest& manifest, uint64_t segmentId);

  // ndn::Name
  // getFileSelectRepoName(const repo::Manifest& manifest, uint64_t segmentId);

  // void
  // getFileRun();

  // void
  // getFileOnManifest(const ndn::Interest& interest, const ndn::Data& data);

  // void
  // getFileOnManifestTimeout(const ndn::Interest& interest);

  // void
  // getFileOnUnversionedData(const ndn::Interest& interest, const ndn::Data& data);

  // bool 
  // getFileVerifyData(const ndn::Data& data);

  // void
  // getFileReadData(const ndn::Data& data);

  // void
  // getFileFetchNextData();

  // void
  // getFileOnTimeout(const ndn::Interest& interest);

  // void
  // signData(ndn::Data& data, bool useDigestSha256);

  void
  putFilePrepareNextData(uint64_t referenceSegmentNo);

  // void
  // signFirstData(ndn::Data& data);

  void
  putFileStartCheckCommand();

  void
  putFileonCheckCommandResponse(const ndn::Interest& interest, const ndn::Data& data);

  void
  putFileStartInsertCommand();

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
  bool m_isFinished;
  std::string m_identityForData;
  std::string m_identityForCommand;
  ndn::Name m_ndnName;

  ndn::Name m_dataPrefix;
  int m_retryCount;

  std::ostream* m_os;
  size_t m_bytes;

  ndn::time::milliseconds timeout;

  ndn::KeyChain m_keyChain;

  ndn::Face m_face;
  ndn::Scheduler m_scheduler;
  using DataContainer = std::map<uint64_t, shared_ptr<ndn::Data>>;
  DataContainer m_data;
  ndn::security::CommandInterestSigner m_cmdSigner;
};

}// namespace difs

#endif  // DIFS_HPP