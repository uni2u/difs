#ifndef REPO_NG_TOOLS_NDNDELFILE_HPP
#define REPO_NG_TOOLS_NDNDELFILE_HPP

#include <ndn-cxx/security/command-interest-signer.hpp>

#include "../src/repo-command-parameter.hpp"
#include <ndn-cxx/face.hpp>

namespace repo {

class NdnDelFile : boost::noncopyable
{
public:
  NdnDelFile(const std::string& repoPrefix,
    const std::string& dataName,
    bool verbose, int interestLifetime, int timeout)
  : m_repoPrefix(repoPrefix)
    , m_dataName(dataName)
    , m_verbose(verbose)
    , m_interestLifetime(interestLifetime)
    , m_timeout(timeout)
    , m_retryCount(0)
    , m_cmdSigner(m_keyChain)
  {}

  void
  run();

private:
  void
  onTimeout(const ndn::Interest& interest);

  void
  deleteData(const ndn::Name& name);

  ndn::Interest
  generateCommandInterest(const ndn::Name& commandPrefix, const std::string& command,
    const RepoCommandParameter& commandParameter);

  void
  onDeleteCommandResponse(const ndn::Interest& interest, const ndn::Data& data);

  void
  onDeleteCommandTimeout(const ndn::Interest& interest);

private:

  ndn::Face m_face;
  ndn::Name m_repoPrefix;
  ndn::Name m_dataName;
  bool m_verbose;
  ndn::time::milliseconds m_interestLifetime;
  ndn::time::milliseconds m_timeout;
  int m_retryCount;

  ndn::KeyChain m_keyChain;
  ndn::security::CommandInterestSigner m_cmdSigner;
};

}  // namespace repo

#endif  // REPO_NG_TOOLS_NDNDELFILE_HPP
// vim: cino=g0,N-s,+0,(s,m1,t0 sw=2
