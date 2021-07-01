#ifndef DIFS_CONSUMER_HPP
#define DIFS_CONSUMER_HPP

#include "manifest/manifest.hpp"

#include <iostream>
#include <map> 
#include <tuple>

#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/security/command-interest-signer.hpp>
#include <ndn-cxx/face.hpp>

namespace difs {

class Consumer : boost::noncopyable {
  public:
	Consumer(repo::Manifest& manifest, std::ostream& os)
	: m_manifest(manifest)
	, m_os(os)
	, m_verbose(false)
	{}

	void
	fetch();

  private:
	void onDataCommandResponse(const ndn::Interest& interest, const ndn::Data& data);

	void onDataCommandTimeout(const ndn::Interest& interest);

	void onDataCommandNack(const ndn::Interest& interest);

  private:
	repo::Manifest m_manifest;
	std::ostream& m_os;
	bool m_verbose;
	std::map<int, const ndn::Block> map;
	int m_currentSegment;
	int m_totalSize;
	int m_retryCount;
	ndn::Face m_face;
};
} // namespace difs

#endif  // DIFS_CONSUMER_HPP