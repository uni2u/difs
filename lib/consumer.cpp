#include "consumer.hpp"
#include <vector>

using namespace difs;

void
Consumer::fetch()
{
  auto repos = m_manifest.getRepos();

  for (auto iter = repos.begin(); iter != repos.end(); ++iter)
  {
	for(int segment_id = iter->start; segment_id <= iter->end; segment_id++) {
		ndn::Interest interest(ndn::Name(iter->name).append("data")
					.append(m_manifest.getName()).appendSegment(segment_id));

		m_face.expressInterest(interest,
								std::bind(&Consumer::onDataCommandResponse, this, _1, _2),
								std::bind(&Consumer::onDataCommandNack, this, _1), // Nack
								std::bind(&Consumer::onDataCommandTimeout, this, _1));
	}
  }

  m_face.processEvents();
}

void
Consumer::onDataCommandResponse(const ndn::Interest& interest, const ndn::Data& data)
{	
	const auto& content = data.getContent();
	content.parse();
	
    ndn::Name::Component segmentComponent = interest.getName().get(-1);
    uint64_t segmentNo = segmentComponent.toSegment();

    ndn::Name::Component endBlockComponent = data.getFinalBlock().value();
    uint64_t endNo = endBlockComponent.toSegment();

	map.insert(std::pair<int, const ndn::Block>(segmentNo, content.get(ndn::tlv::Content)));

	if(map.size() - 1 == endNo) {
		for(auto iter = map.begin(); iter != map.end(); iter++) {
			m_os.write(reinterpret_cast<const char *>(iter->second.value()), iter->second.value_size());
			m_totalSize += iter->second.value_size();
			m_currentSegment += 1;
		}

		std::cerr << "INFO: End of file is reached" << std::endl;
		std::cerr << "INFO: Total # of segments received: " << m_currentSegment << std::endl;
		std::cerr << "INFO: Total # bytes of content received: " << m_totalSize << std::endl;
	}
}

void
Consumer::onDataCommandTimeout(const ndn::Interest& interest)
{
	if(m_retryCount++ < 3) {
		fetch();
    	if (m_verbose) {
     	  std::cerr << "TIMEOUT: retransmit interest for " << interest.getName() << std::endl;
    	}
	} else {
		std::cerr << "TIMEOUT: last interest sent" << std::endl
		<< "TIMEOUT: abort fetching after " << 3 << " times of retry" << std::endl;
	}
}

void
Consumer::onDataCommandNack(const ndn::Interest& interest)
{
	if(m_retryCount++ < 3) {
		fetch();
    	if (m_verbose) {
     	  std::cerr << "TIMEOUT: retransmit interest for " << interest.getName() << std::endl;
    	}
	} else {
		std::cerr << "NACK: last interest sent" << std::endl
		<< "NACK: abort fetching after " << 3 << " times of retry" << std::endl;
	}
}