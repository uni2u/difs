#include "util.hpp"

#include <ndn-cxx/security/command-interest-signer.hpp>
#include <ndn-cxx/security/hc-key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>

#if BOOST_VERSION == 107400
#include <boost/uuid/detail/sha1.hpp>
#else
#include <boost/uuid/sha1.hpp>
#endif

#include <iostream>

namespace repo {
namespace util {

ndn::Interest
generateCommandInterest(
    const ndn::Name& commandPrefix, const std::string& command,
    const RepoCommandParameter& commandParameter,
    milliseconds interestLifetime)
{

  ndn::HCKeyChain hcKeyChain;
  ndn::security::CommandInterestSigner cmdSigner(hcKeyChain);

  Name cmd = commandPrefix;
  cmd
    .append(command)
    .append(commandParameter.wireEncode());
  ndn::Interest interest;

  interest = cmdSigner.makeCommandInterest(cmd);

  interest.setInterestLifetime(interestLifetime);
  return interest;
}

} // namespace util
} // namespace repo
