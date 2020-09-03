#include "util.hpp"

#include <ndn-cxx/security/command-interest-signer.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>

#include <boost/uuid/detail/sha1.hpp>
#include <iostream>

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

std::array<int, 5>
calcHash(uint8_t * buffer, size_t length)
{
    boost::uuids::detail::sha1 sha1;
    unsigned hashBlock[5] = {0};
    sha1.process_bytes(buffer, length);
    sha1.get_digest(hashBlock);

    std::array<int, 5> hash;
    for (int i = 0; i < 5; i += 1) {
      hash[i] = hashBlock[i];
    }
    return hash;
}

} // namespace util
} // namespace repo
