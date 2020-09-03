#ifndef REPO_UTIL
#define REPO_UTIL

#include <ndn-cxx/face.hpp>
#include "repo-command-parameter.hpp"

namespace repo {
namespace util {

static const int HASH_SIZE = sizeof(unsigned) * 5;

ndn::Interest
generateCommandInterest(
    const ndn::Name& commandPrefix, const std::string& command,
    const RepoCommandParameter& commandParameter,
    milliseconds interestLifetime);


std::array<uint8_t, HASH_SIZE>
calcHash(uint8_t* buffer, size_t length);
} // namespace util
} // namespace repo
#endif // REPO_UTIL

