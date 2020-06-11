#ifndef REPO_UTIL
#define REPO_UTIL

#include <ndn-cxx/face.hpp>
#include "repo-command-parameter.hpp"

namespace repo {
namespace util {

ndn::Interest
generateCommandInterest(
    const ndn::Name& commandPrefix, const std::string& command,
    const RepoCommandParameter& commandParameter,
    milliseconds interestLifetime);
} // namespace util
} // namespace repo
#endif // REPO_UTIL

