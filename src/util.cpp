#include "util.hpp"

#include <ndn-cxx/security/command-interest-signer.hpp>
#include <ndn-cxx/security/hc-key-chain.hpp>
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


std::array<uint8_t, HASH_SIZE>
convertHash(unsigned hashBlock[5]) {
  std::array<uint8_t, HASH_SIZE> hash;
  auto innerLoopCount = sizeof(unsigned) / sizeof(uint8_t);
  for (int i = 0; i < 5; i += 1) {
    for (int j = 0; j < (int)innerLoopCount; j += 1) {
      hash[i * innerLoopCount + j] = hashBlock[i] >> (8 * (innerLoopCount - j - 1));
    }
  }
  return hash;
}

std::array<uint8_t, HASH_SIZE>
calcHash(uint8_t * buffer, size_t length)
{
  boost::uuids::detail::sha1 sha1;
  unsigned hashBlock[5] = {0};
  sha1.process_bytes(buffer, length);
  sha1.get_digest(hashBlock);

  return convertHash(hashBlock);
}

bool
verifyHash(const uint8_t * buffer, size_t length, std::array<uint8_t, HASH_SIZE> hash)
{
  boost::uuids::detail::sha1 sha1;
  unsigned hashBlock[5] = {0};
  sha1.process_bytes(buffer, length);
  sha1.get_digest(hashBlock);

  auto calcedHash = convertHash(hashBlock);

  return calcedHash == hash;
}

} // namespace util
} // namespace repo
