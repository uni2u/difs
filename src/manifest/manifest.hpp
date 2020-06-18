#ifndef REPO_MANIFEST_MANIFEST_HPP
#define REPO_MANIFEST_MANIFEST_HPP

#include <ndn-cxx/face.hpp>

namespace repo {

class Manifest
{
public:
  struct Repo
  {
    std::string name;
    int start;
    int end;
  };

public:
  Manifest(std::string name, int startBlockId, int endBlockId)
  : m_name(name)
  , m_repos(std::list<Repo>())
  , m_startBlockId(startBlockId)
  , m_endBlockId(endBlockId)
  {}

public:

  static std::string
  getHash(const std::string name);

  static ndn::Name
  getManifestStorage(
    ndn::Name const& prefix,
    const std::string name,
    unsigned int clusterSize);

  std::string
  toJson() const;

  static Manifest
  fromJson(std::string json);

  std::string
  toInfoJson();

  static Manifest
  fromInfoJson(std::string json);

  ndn::Name
  getManifestStorage(ndn::Name const& prefix, int clusterSize);

  std::list<Repo>
  getRepos() const;

  void
  appendRepo(std::string repoName, int start, int end);

  std::string
  getName() const;

  std::string
  getHash() const;

  void
  setHash(std::string digest);

  int
  getStartBlockId();

  void
  setStartBlockId();

  int
  getEndBlockId();

  void
  setEndBlockId();

private:
  std::string m_name;
  std::string m_hash;
  std::list<Repo> m_repos;

  int m_startBlockId;
  int m_endBlockId;

private:
  std::string
  makeHash() const;
};

} // namespace repo
#endif // REPO_MANIFEST_MANIFEST_HPP
// vim: cino=g0,N-s,+0,i-s sw=2
