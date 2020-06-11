#include "util.hpp"

#include <ndn-cxx/security/command-interest-signer.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>

namespace repo {
namespace util {

ndn::Interest
generateCommandInterest(
    const ndn::Name& commandPrefix, const std::string& command,
    const RepoCommandParameter& commandParameter,
    milliseconds interestLifetime)
{

  ndn::KeyChain keyChain;
  ndn::security::CommandInterestSigner cmdSigner(keyChain);

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
