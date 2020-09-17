
#include <iostream>
#include <vector>
#include <map>
#include <boost/program_options.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/validator-config.hpp>
#include <ndn-cxx/security/transform/public-key.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/util/time.hpp>
#include <ndn-cxx/util/config-file.hpp>
#include <ndn-cxx/security/command-interest-signer.hpp>
#include <ndn-cxx/security/v2/certificate-request.hpp>
#include <ndn-cxx/security/v2/certificate.hpp>
#include <ndn-cxx/mgmt/nfd/controller.hpp>
#include <ndn-cxx/mgmt/nfd/control-command.hpp>
#include <ndn-cxx/mgmt/nfd/control-parameters.hpp>
#include <ndn-cxx/mgmt/nfd/status-dataset.hpp>
#include <ndn-cxx/lp/tags.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include <ndn-cxx/encoding/buffer-stream.hpp>
#include <ndn-cxx/security/transform/buffer-source.hpp>
#include <ndn-cxx/security/transform/stream-sink.hpp>
#include <ndn-cxx/security/transform/block-cipher.hpp>


#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <ctime>
#include <iostream>
#include <fstream>
#include  <boost/property_tree/info_parser.hpp>
#include "conf-parameter.hpp"

using boost::asio::ip::udp;

using namespace std;
using namespace ndn;
using namespace ndn::nfd;
using namespace ndn::security::v2;
using namespace ndn::security::transform;
using std::cerr;
using std::cout;
using std::endl;
namespace po = boost::program_options;
using namespace boost::program_options;

ndn::Name g_obuKeyName ("/etri/obu/KEY");

enum { RSU_OBU_NOTI=1, OBU_RSU_HORIZON_REQ=3, OBU_RSU_HORIZON_RES=4, OBU_RSU_EVENT=5 };

typedef struct
{
     uint8_t packet_type;
     uint8_t packet_id;
     uint8_t packet_sub_id;
     uint16_t crc16;
     uint16_t packet_len;
     uint8_t f1;
     uint8_t f2;
     uint8_t f3;
} __attribute__((packed)) v2x_hdr, *v2x_hdr_ptr;

string str_packet_type[6] = { "Dummy", "RSU-OBU-NOTI", "Pub/Sub", "OBU-RSU-HORIZON-REQ", "OBU-RSU-HORIZON-RES", "OBU-RSU-EVENT"};

const uint8_t key_obu[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	     0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
		   };
const uint8_t iv[] = {
	     0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	     0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
		   };


class NdnV2x
{
	public:
        NdnV2x(Face& face, v2x::ConfParameter &param, shared_ptr<spdlog::logger> log)
            :m_io_service(face.getIoService())
             ,m_face(face)
             ,m_validator(m_face)
             ,m_sockV2x(m_io_service, udp::endpoint(udp::v4(), param.getHostPcPort()))
        , m_confParam(param)
             , m_controller(m_face, m_keyChain)
             ,m_scheduler(m_io_service)
			 ,m_logger(log)
    {
		m_logger->info("NdnV2x's Validation File: {}" , m_confParam.getValidFileName() );
		std::cout << "NdnV2x's Listening Port: " << m_confParam.getHostPcPort() << std::endl;

		auto id = m_keyChain.getPib().getDefaultIdentity();

		auto  buf = id.getDefaultKey().getDefaultCertificate().getPublicKey();//(id.getDefaultKey().getName());
		PublicKey pKey;
	   pKey.loadPkcs8(buf.data(), buf.size());

		m_controller.fetch<
			ndn::nfd::FaceDataset>(
				std::bind(&NdnV2x::processFaceDataset, this, _1), 
				std::bind(&NdnV2x::onFaceDatasetFetchTimeout, this, _1, _2, 0));


		//config validation rule from conf file
		m_validator.load(m_confParam.getValidFileName());

		auto keyPrefix = m_keyChain.getPib().getDefaultIdentity().getName();


		keyPrefix.append(Certificate::KEY_COMPONENT);

		m_face.setInterestFilter( keyPrefix,
				bind(&NdnV2x::onKeyInterest,     this, _1, _2),
				bind(&NdnV2x::onPrefixRegSuccess, this, _1),
				bind(&NdnV2x::onRegisterFailed, this, _1, _2),
				security::SigningInfo(),
				nfd::ROUTE_FLAG_CAPTURE
				);

		if(m_confParam.getV2xMode()==v2x::V2X_MODE_OBU){

			printf("\n\n\nNdnV2x's Mode: OBU with Id[%d]\n" , m_confParam.getCarId());
		m_face.setInterestFilter( "/etri/rsu",
				bind(&NdnV2x::onInterest,     this, _1, _2),
				bind(&NdnV2x::onPrefixRegSuccess, this, _1),
				bind(&NdnV2x::onRegisterFailed, this, _1, _2),
				security::SigningInfo(),
				nfd::ROUTE_FLAG_CAPTURE
				);

		}else{ 
			printf("\n\n\nNdnV2x's Mode: RSU with Id[%d]\n" , m_confParam.getRsuId());

			m_face.setInterestFilter(m_confParam.getPrefixEvent(),
					bind(&NdnV2x::onInterest,     this, _1, _2),
					bind(&NdnV2x::onPrefixRegSuccess, this, _1),
					bind(&NdnV2x::onRegisterFailed, this, _1, _2),
				security::SigningInfo(),
				nfd::ROUTE_FLAG_CAPTURE
					);

			m_face.setInterestFilter(m_confParam.getPrefixHorizon(),
					bind(&NdnV2x::onInterest,     this, _1, _2),
					bind(&NdnV2x::onPrefixRegSuccess, this, _1),
					bind(&NdnV2x::onRegisterFailed, this, _1, _2),
				security::SigningInfo(), nfd::ROUTE_FLAG_CAPTURE);

		}

		m_controller.start<ndn::nfd::FaceUpdateCommand>(
				ndn::nfd::ControlParameters()
				.setFlagBit(ndn::nfd::FaceFlagBit::BIT_LOCAL_FIELDS_ENABLED, true),
				bind(&NdnV2x::onFaceIdIndicationSuccess, this, _1),
				bind(&NdnV2x::onFaceIdIndicationFailure, this, _1));

		m_sockV2x.async_receive_from(
				boost::asio::buffer(m_udp_data, max_length), m_HostPcEndPoint,
				boost::bind(&NdnV2x::handle_receive_from, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred)
				);
	}

		void
			onFaceIdIndicationSuccess(const ndn::nfd::ControlParameters& cp)
			{
				cout <<"Successfully enabled incoming face id indication"
					<< "for face id " << cp.getFaceId() << endl;
			}

		void
			onFaceIdIndicationFailure(const ndn::nfd::ControlResponse& cr)
			{
				std::ostringstream os;
				os << "Failed to enable incoming face id indication feature: " <<
					"(code: " << cr.getCode() << ", reason: " << cr.getText() << ")";

				os.str();
			}


        int ndnv2x_auto_car_event(size_t pkt_len)
		{
			std::cout << "ndnv2x_auto_car_event:::: raw-data(" << pkt_len << ")" << std::endl;

			ndn::security::CommandInterestSigner signer(m_keyChain);
			vector<uint8_t> appData(m_udp_data, m_udp_data+pkt_len);

			Name name("/etri/auto-car");

			string str_car_id("/car");
			str_car_id += std::to_string(m_confParam.getCarId());

			name.append(str_car_id);
			name.append("car-event");

			Interest interest = signer.makeCommandInterestWithAppParams(
					name, appData, signingByIdentity(m_keyChain.getPib().getDefaultIdentity()));

			interest.setMustBeFresh(true);
			interest.setCanBePrefix(true);

			cout << "Express interest " << interest.toUri() << "\n" << endl;

			m_face.expressInterest(interest, nullptr, nullptr, nullptr);
			return 0;
		}

        int ndnv2x_my_global_path_with_privacy()
		{
         	std::cout << __func__ << " RSU" << std::endl;

			Interest interest("/etri/c-horizon/request-key-name");
			interest.setMustBeFresh(true);
			m_face.expressInterest(interest,
					bind(&NdnV2x::onRequestKeyName, this, _1, _2),
					bind(&NdnV2x::onNack, this, _1, _2),
					bind(&NdnV2x::onTimeout, this, _1));

				return 0;
		}

		void onRequestKeyName(const Interest& interest, const Data& data)
		{

			cout << __func__ << ", Data: " << data.getName() << endl;

			//ndn::Block blk = data.getContent();

			ndn::Block kl(data.getContent().value(), data.getContent().size());

			Name name(kl);

			cout << "CERT-----::" << name << endl;

			auto it = m_certMap.find(name);

			if(it == m_certMap.end()){
				cout << __func__ << ", Line:" << __LINE__ << endl;

				CertificateRequest cert(name);
				cert.interest.setMustBeFresh(true);
				m_face.expressInterest(cert.interest,
						bind(&NdnV2x::onCertificate, this, _1, _2),
						bind(&NdnV2x::onNack, this, _1, _2),
						bind(&NdnV2x::onTimeout, this, _1));
				return;
			}

			keep_going_gpp(it->second);
#if 0
			PrivateKey sKey;
			sKey.loadPkcs1Base64(reinterpret_cast<const uint8_t*>(dataSet.privateKeyPkcs1.data()),
					dataSet.privateKeyPkcs1.size());
			auto decrypted = sKey.decrypt(ct->data(), ct->size());
			cout << "decrypted: " << decrypted->data() << std::endl;
#endif

		}

		void onCertificate(const Interest& certInterest, const Data& certData)
		{
			cout << __func__ << ", Line:" << __LINE__ << endl;
			ndn::security::v2::Certificate cert(certData);

			m_certMap.insert(std::pair<ndn::Name, Certificate>(certInterest.getName(), cert));
			cout << "m_certMap's Size: " << m_certMap.size() << endl;

			keep_going_gpp(cert);
		}

		void keep_going_gpp(Certificate rsuCert)
		{
			cout << __func__ << ", Line:" << __LINE__ << endl;
			auto rsuKey = rsuCert.getPublicKey();
			auto obuKey = m_keyChain.getPib().getDefaultIdentity().getDefaultKey().getDefaultCertificate().getPublicKey();

			cout << "rsuKey's Size: " << rsuKey.size() << endl;
			cout << "obuKey's Size: " << obuKey.size() << endl;

			PublicKey rsuPublicKey;
			rsuPublicKey.loadPkcs8(rsuKey.data(), rsuKey.size());

			v2x_hdr_ptr hdr = (v2x_hdr_ptr)m_udp_data;
			cout << "HDR Size: " << hdr->packet_len << endl;

			OBufferStream os;
			bufferSource( m_udp_data, hdr->packet_len ) >>
				blockCipher(BlockCipherAlgorithm::AES_CBC, CipherOperator::ENCRYPT,
						key_obu, sizeof(key_obu), iv, sizeof(iv)) >> streamSink(os);

			ConstBufferPtr cipher_udp_ptr = os.buf();
			const uint8_t* cipher_udp = cipher_udp_ptr->data();
			size_t cipher_udp_len =     cipher_udp_ptr->size();

			//string keystring = m_confParam.getPassPhrase();
			//cout << "key-string:" << keystring << ", " << keystring.length() << endl;
			cout << "key-obu-array:" << sizeof(key_obu) << endl;
#if 1

			ConstBufferPtr cipherKey = rsuPublicKey.encrypt(
					//reinterpret_cast<const uint8_t*>(keystring.c_str()), sizeof(keystring.length()));
					key_obu, sizeof(key_obu));

			cout << "cipher_udp_len: " << cipher_udp_len << endl;
			cout << "cipherKey: " << cipherKey->size() << endl;

			Buffer encryptedData(cipher_udp_len + cipherKey->size());

			memcpy(encryptedData.data(), cipherKey->data(), cipherKey->size());
			memcpy((encryptedData.data()+(cipherKey->size())), cipher_udp, cipher_udp_len);

			Interest interest ("/etri/c-horizon/my-global-path");
            interest.setMustBeFresh(true);

			interest.setApplicationParameters(encryptedData.data(), encryptedData.size());

			m_face.expressInterest(interest,
					bind(&NdnV2x::onData, this, _1, _2),
					bind(&NdnV2x::onNack, this, _1, _2),
					bind(&NdnV2x::onTimeout, this, _1));
#endif

		}

        int ndnv2x_my_global_path()
        {
         	std::cout << __func__ << " RSU" << std::endl;

            vector<uint8_t> appData(m_udp_data, m_udp_data+m_rcvd_data_len);

            Name name("/etri/c-horizon/my-global-path");

			time::milliseconds timestamp = time::toUnixTimestamp(time::system_clock::now());
			name.append(name::Component::fromNumber(timestamp.count()));

            Interest interest(name);
            interest.setMustBeFresh(true);
            interest.setCanBePrefix(true);

            cout << "Express interest " << interest.toUri() << "\n" << endl;

                m_face.expressInterest(interest,
                        bind(&NdnV2x::onData, this,  _1, _2),
                        bind(&NdnV2x::onNack, this, _1, _2),
                        bind(&NdnV2x::onTimeout, this, _1));

         return 0;
        }

		void handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd)
		{
			v2x_hdr_ptr hdr = (v2x_hdr_ptr)m_udp_data;
				printf("\n\nNDN-V2X: UDP Rcv: %ld With PacketType: %d\n", bytes_recvd, hdr->packet_type);

			if (!error && bytes_recvd > 0){

				m_rcvd_data_len = bytes_recvd;

				if(hdr->packet_len == bytes_recvd){

					if( hdr->packet_type == OBU_RSU_EVENT){
						ndnv2x_auto_car_event(bytes_recvd);
					}else if( hdr->packet_type == OBU_RSU_HORIZON_REQ){

						if(m_confParam.getHorizonMode()==v2x::V2X_HORIZON_NORMAL)
							ndnv2x_my_global_path( );
						else
							ndnv2x_my_global_path_with_privacy();


					}else if( hdr->packet_type == RSU_OBU_NOTI){

						ndn::security::CommandInterestSigner signer(m_keyChain);
						Name rsuKey = "/etri/rsu0";
						vector<uint8_t> appData(m_udp_data, m_udp_data+m_rcvd_data_len);

						Name name("/etri/rsu/rsu0/noti/ldm");

						Interest interest = signer.makeCommandInterestWithAppParams(name, appData, signingByIdentity(rsuKey));
						interest.setMustBeFresh(true);
						interest.setCanBePrefix(true);

						cout << "Express interest " << interest.toUri() << "\n" << endl;

						m_face.expressInterest(interest, nullptr, nullptr, nullptr);
					}else{
						printf("NDN-V2X Error: Unknown PacketType: %d\n", hdr->packet_type );

					}
				}
			} else {
				printf("NDN-V2X Error: Packe hdr: %d/ Socket Recvd:%ld\n", hdr->packet_len, bytes_recvd );
				m_rcvd_data_len = 0;
			}

			m_sockV2x.async_receive_from(
					boost::asio::buffer(m_udp_data, max_length), m_HostPcEndPoint,
					boost::bind(&NdnV2x::handle_receive_from, this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred)
					);
		}

        void handle_send_to(const boost::system::error_code& error, size_t bytes_sent)
        {
            m_sockV2x.async_receive_from(
                    boost::asio::buffer(m_udp_data, max_length), m_HostPcEndPoint,
                    boost::bind(&NdnV2x::handle_receive_from, this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred)
                    );

        }

    private:
        void onKeyInterest(const InterestFilter&, const Interest&interest)
        {
            std::cout << "Got interest for certificate. Interest: " << std::endl;

            try{

                Name identity = extractIdentityFromKeyName(interest.getName());

                const auto cert = 
                    //		m_keyChain.getPib().getIdentity(identity).getDefaultKey().getDefaultCertificate(); 
                    m_keyChain.getPib().getDefaultIdentity().getDefaultKey().getDefaultCertificate(); 

                m_face.put(cert);

            } catch(std::exception& e) {
                std::cout << "Certificate is not found for: " << interest << std::endl;
            }
        }

        void onPrefixRegSuccess(const ndn::Name& name)
        {
            std::cout << "Prefix: " << name << " registration is successful."<< std::endl;
        }

        void onInterest(const InterestFilter&, const Interest& interest)
        {
            cout << ">>> Receive interest and validation start..." << endl;
            cout << interest.getName() << endl;

			if( !interest.getName().compare(0,3, "/etri/c-horizon/request-key-name")){

				if(m_confParam.getV2xMode() != v2x::V2X_MODE_RSU){
                	std::cout << "Error OBU Rcv C-Horizon/RequestID..." << std::endl;
					return;
				}

				ndn::Data data = ndn::Data(interest.getName());
                data.setFreshnessPeriod(1_ms);

				ndn::Name name = m_keyChain.getPib().getDefaultIdentity().getDefaultKey().getName();

				string key = name.toUri();

                data.setContent( name.wireEncode() );

                auto cert = m_keyChain.getPib().getDefaultIdentity().getDefaultKey().getDefaultCertificate(); 
                m_keyChain.sign(data, signingByCertificate(cert));

                // Return Data packet to the requester
                std::cout << "<< D: " << data << std::endl;

                m_face.put(data);

				return;
			}else if( !interest.getName().compare(0,3, "/etri/c-horizon/my-global-path")){

				if(m_confParam.getV2xMode() != v2x::V2X_MODE_RSU){

					cout << "RKKKKeceive data and validation start\n" << endl;
					return;
				}

				ndn::Block ap = interest.getApplicationParameters();
				ndn::Data data(interest.getName());
				data.setFreshnessPeriod(1_ms);
				uint8_t ResGpp[1024];
				v2x_hdr_ptr hdr = (v2x_hdr_ptr)ResGpp;
				string path("RSU0->RSU2->RSU1->Home:::Timestamp -> ");
				std::time_t result = std::time(nullptr);

				if(m_confParam.getHorizonMode()==v2x::V2X_HORIZON_NORMAL){

					path += "NORAML Mode: ";

					path +=  std::asctime(std::localtime(&result));

					memcpy(ResGpp+sizeof(v2x_hdr), path.c_str(), path.length());
					data.setContent( ResGpp, sizeof(ResGpp));
					// Return Data packet to the requester

				}else{
					path += "Privacy Mode: ";
					path +=  std::asctime(std::localtime(&result));

					printf("\nThe RSU(%d), is receiving(%s)\n" , m_confParam.getRsuId() ,interest.getName().toUri().c_str() );

					auto cert = m_keyChain.getPib().getDefaultIdentity().getDefaultKey().getDefaultCertificate(); 

					ndn::Block pa = interest.getApplicationParameters();

					ConstBufferPtr decryptedPassPhrase= m_keyChain.getTpm().decrypt(pa.value(), 256,//pa.value_size(),
							m_keyChain.getPib().getDefaultIdentity().getDefaultKey().getName()
							);

					auto pp = decryptedPassPhrase->data();

					for(int i=0;i<16;i++)
						printf("%d-%d ", i, pp[i]);

					cout << "decrypted Size: " << decryptedPassPhrase->size() << endl;

					OBufferStream os;
					bufferSource(pa.value()+256, pa.value_size()-256) >>
						blockCipher(BlockCipherAlgorithm::AES_CBC, CipherOperator::DECRYPT,
						pp, decryptedPassPhrase->size(), iv, sizeof(iv)) >> streamSink(os);

					ConstBufferPtr decryptedUdp = os.buf();
					const uint8_t *req = decryptedUdp->data();

					hdr = (v2x_hdr_ptr)req;
					printf ( "\nPacketType:%d/PacketLen:%d\n" , hdr->packet_type , hdr->packet_len);

					hdr = (v2x_hdr_ptr)ResGpp;
					hdr->packet_type = OBU_RSU_HORIZON_RES;
					hdr->packet_len = sizeof(ResGpp);

					memcpy(ResGpp+sizeof(v2x_hdr), path.c_str(), path.length());

					OBufferStream os1;
					bufferSource( ResGpp, hdr->packet_len ) >>
						blockCipher(BlockCipherAlgorithm::AES_CBC, CipherOperator::ENCRYPT,
							pp, decryptedPassPhrase->size(), iv, sizeof(iv)) >> streamSink(os1);

					data.setContent( os1.buf()->data(), os1.buf()->size());
				}

				auto cert = m_keyChain.getPib().getDefaultIdentity().getDefaultKey().getDefaultCertificate(); 
				m_keyChain.sign(data, signingByIdentity(m_keyChain.getPib().getDefaultIdentity()));

				std::cout << "<< D: " << data << std::endl;
				m_face.put(data);

				return;
			}


            m_validator.validate(interest,
                    bind(&NdnV2x::onInterestValidationSuccess, this, _1),
                    bind(&NdnV2x::onInterestValidationFailure, this, _1, _2)
                    );
        }

        void onRegisterFailed(const Name& prefix, const std::string& reason)
        {
            cout << "Register failed for prefix " << reason << endl;
            m_face.shutdown();
        }

        void onData(const Interest& interest, const Data& data)
        {
            cout << "Receive data and validation start\n" << endl;

#if 1
            m_validator.validate(data,
                    bind(&NdnV2x::onDataValidationSuccess, this, _1),
                    bind(&NdnV2x::onValidationFailure, this, _1, _2)
                    );
#endif
        }

        void onTimeout(const Interest& interest)
        {
            cout << "Time out for interest " << interest.getName().toUri() << endl;
            //m_face.shutdown();
        }

        void onNack(const Interest& interest, const lp::Nack& nack) 
        {
            std::cout << interest.getName() << " Received Nack with reason " << nack.getReason() << std::endl;
            //m_face.shutdown();
        }

    private:  // validation callback
        void onInterestValidationSuccess(const Interest& interest)
        {
            cout << "\n++++ Got interest packet with name " << interest.getName().toUri() << "\n" << endl;

            if(!interest.getName().compare(0, 2, m_confParam.getPrefixHorizon())){

				if(m_confParam.getV2xMode() != v2x::V2X_MODE_RSU){
					std::cout << "OBU rcv Event... Error" << std::endl;
					return;
				}

            	printf("\nThe RSU(%d), is receiving(%s)\n" , m_confParam.getRsuId() ,interest.getName().toUri().c_str() );

                auto cert = m_keyChain.getPib().getDefaultIdentity().getDefaultKey().getDefaultCertificate(); 

                ndn::Block pa = interest.getApplicationParameters();

				ConstBufferPtr decrypted= m_keyChain.getTpm().decrypt(pa.value(), 256,//pa.value_size(),
						m_keyChain.getPib().getDefaultIdentity().getDefaultKey().getName()
						);

				auto buf = decrypted->data();

				for(int i=0;i<16;i++)
					printf("%d ", buf[i]);

				PublicKey pKey;

				v2x_hdr_ptr hdr = (v2x_hdr_ptr)(decrypted->data());

				uint16_t packet_len = hdr->packet_len;

				cout << "pKey's Size: " << decrypted->size()-packet_len << endl;


				if( hdr->packet_type != OBU_RSU_HORIZON_REQ ){
            		cout << "\nError, Packet Type:" << hdr->packet_type << endl;
					return;
				}

				pKey.loadPkcs8(decrypted->data() + packet_len, decrypted->size()-packet_len);

				// make global-path plan

				Buffer gpp(2048);

				hdr = (v2x_hdr_ptr)gpp.data();

				hdr->packet_type = OBU_RSU_HORIZON_RES;
				hdr->packet_len = 2048;

				ConstBufferPtr ct = pKey.encrypt(gpp.data(), hdr->packet_len);

				ndn::Data data(interest.getName());
                data.setFreshnessPeriod(1_ms);

                data.setContent(gpp.data(), gpp.size());

                m_keyChain.sign(data, signingByCertificate(cert));

                // Return Data packet to the requester
                std::cout << "<< D: " << data << std::endl;

                m_face.put(data);

			}else if(!interest.getName().compare(0, 2, m_confParam.getPrefixEvent())){
				if(m_confParam.getV2xMode() != v2x::V2X_MODE_RSU){
					std::cout << "OBU rcv Event... Error" << std::endl;
					return;
				}

                ndn::Block pa = interest.getApplicationParameters();
				//v2x_hdr_ptr hdr = (v2x_hdr_ptr)pa.value();
				cout << "RSU rcv Event: " << pa.value()+sizeof(v2x_hdr) << std::endl;

			}else if(!interest.getName().compare(2, 1, "/noti") ){

				if(m_confParam.getV2xMode() != v2x::V2X_MODE_OBU){
					std::cout << "Error rcv NOtification..." << std::endl;
					return;
				}

                ndn::Block pa = interest.getApplicationParameters();

				size_t len = pa.value_size();
				memset(m_udp_data, '\0', len);
				memcpy(m_udp_data, pa.value(), len);

				cout << "RSU rcv Noti: " << pa.value()+sizeof(v2x_hdr) << std::endl;

        		auto pc_host =  udp::endpoint(
						//boost::asio::ip::address::from_string(m_confParam.getHostPcAddr()) , m_confParam.getHostPcPort()
						boost::asio::ip::address::from_string("127.0.0.1") , 2003
						);

                m_sockV2x.async_send_to(
                    boost::asio::buffer(m_udp_data, len), pc_host,
                    boost::bind(&NdnV2x::handle_send_to, this,
                     boost::asio::placeholders::error,
                     boost::asio::placeholders::bytes_transferred)
                    );

            }else{
                ndn::Block pa = interest.getApplicationParameters();

                std::cout << "<< Received Parameters's Size: " << pa.value_size() << std::endl;
            }

        }

        void onDataValidationSuccess(const Data& data)
        {
			cout << "\nData Validation success\n" << endl;
			cout << "Got data packet with name " << data.getName().toUri() << "\n" << endl;

			if(! data.getName().compare(0,3, "/etri/c-horizon/my-global-path")){

				if( m_confParam.getHorizonMode() == v2x::V2X_HORIZON_PRIVACY){

					OBufferStream os;
					bufferSource(data.getContent().value(), data.getContent().value_size()) >>
						blockCipher(BlockCipherAlgorithm::AES_CBC, CipherOperator::DECRYPT,
						key_obu, sizeof(key_obu), iv, sizeof(iv)) >> streamSink(os);

					v2x_hdr_ptr hdr = (v2x_hdr_ptr)os.buf()->data();

					printf( "hdr->type:%d/%d\n" , hdr->packet_type , hdr->packet_len);

					printf("GP Plan: %s\n", os.buf()->data()+sizeof(v2x_hdr));
#if 0 //must change 0 to 1
					m_sockV2x.async_send_to(
							boost::asio::buffer(
								os.buf()->data(), os.buf()->size()), 
							m_HostPcEndPoint,
							boost::bind(&NdnV2x::handle_send_to, this,
								boost::asio::placeholders::error,
								boost::asio::placeholders::bytes_transferred));
#endif

				}else{
					printf("GP Plan: %s\n", data.getContent().value()+sizeof(v2x_hdr));

#if 0 
					m_sockV2x.async_send_to(
							boost::asio::buffer(
								data.getContent().value(), data.getContent().value_size()), 
							m_HostPcEndPoint,
							boost::bind(&NdnV2x::handle_send_to, this,
								boost::asio::placeholders::error,
								boost::asio::placeholders::bytes_transferred));
#endif

				}
			}
		}

		void processFaceDataset(const std::vector<ndn::nfd::FaceStatus>& faces)
		{

			cout << __func__ << endl;
			for (const auto& faceStatus : faces) {
				if( m_confParam.getWaveInterface()==faceStatus.getLocalUri() ){

					cout << "FaceUri: " << faceStatus.getLocalUri() <<
						" FaceId: "<< faceStatus.getFaceId() << endl;

					ControlParameters parameters;
					CommandOptions options;
					parameters.setName("/etri");
					parameters.setFaceId(faceStatus.getFaceId());
					m_controller.start<RibRegisterCommand>(parameters, nullptr, nullptr, options);
					break;
				}
			}


		}

		void onFaceDatasetFetchTimeout(uint32_t code, const std::string& msg, uint32_t nRetriesSoFar) 
		{
		}

        void onInterestValidationFailure(const Interest& interest, const ValidationError& error)
        {
            cout << "Interest Validation fail : " << error << endl;
        }

        void onValidationFailure(const Data& data, const ValidationError& error)
        {
            cout << "Validation fail : " << error << endl;
        }

        boost::asio::io_service& m_io_service;
        ndn::Face& m_face;
        KeyChain m_keyChain;
        ValidatorConfig m_validator;
        udp::socket m_sockV2x;
        udp::endpoint m_HostPcEndPoint;

        enum { max_length = 8800  };
        uint8_t m_udp_data[max_length];
		int m_rcvd_data_len;

		v2x::ConfParameter& m_confParam;

        ndn::nfd::Controller m_controller;
        std::map<ndn::Name, Certificate> m_certMap;
        Scheduler m_scheduler;
		shared_ptr<spdlog::logger> m_logger;


};

// A helper function to simplify the main part.
template<class T>
ostream& operator<<(ostream& os, const vector<T>& v)
{
    copy(v.begin(), v.end(), ostream_iterator<T>(os, " "));
    return os;
}

int main(int argc, char** argv)
{
    try {

        Face face;
        string configFile;
        po::options_description desc("Allowed options");

        desc.add_options()
            ("help,h", "produce help message")
            ("configuration,c", po::value<std::string>(&configFile)->required(), "Rule Check")
        ;

        po::positional_options_description p;

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
              options(desc).positional(p).run(), vm);
		po::notify(vm); 

		shared_ptr<spdlog::logger> log = spdlog::daily_logger_mt("dcn-daily-logger", "/tmp/v2x.log", 8, 0);

		std::ifstream inputFile;
		inputFile.open(configFile.c_str());

		using ConfigSection = boost::property_tree::ptree;

		ConfigSection pt; 
		try {
			boost::property_tree::read_info(inputFile, pt);
		}
		catch (const boost::property_tree::info_parser_error& error) {
			std::stringstream msg;
			std::cerr << "Failed to parse configuration file " << std::endl;
			std::cerr << configFile<< std::endl;
			return false;
		}

		v2x::ConfParameter confParam;

		for (const auto& tn : pt) {
			const ConfigSection& section = tn.second;
			if( tn.first == "general"){

				if( section.get<std::string>("v2x-mode") == "obu" )
					confParam.setV2xMode(v2x::V2X_MODE_OBU);
				else
					confParam.setV2xMode(v2x::V2X_MODE_RSU);

				confParam.setHostPcAddr(section.get<std::string>("host-pc-ip")); 
				short port = section.get<short>("host-pc-port", 2002);
				confParam.setHostPcPort(port); 

				confParam.setValidFileName(section.get<std::string>("validator-path")); 
                std::string h_mode = section.get<std::string>("c-horizon-mode"); 
                if(h_mode=="normal")
                    confParam.setHorizonMode(v2x::V2X_HORIZON_NORMAL);
                else
                    confParam.setHorizonMode(v2x::V2X_HORIZON_PRIVACY);

				confParam.setWaveInterface(section.get<std::string>("wave-interface")); 
//				confParam.setPassPhrase(section.get<std::string>("c-horizon-passphrase")); 

			}else if (tn.first == "v2x-id"){
				int8_t id;
				if(confParam.getV2xMode()==v2x::V2X_MODE_RSU){
					id = section.get<int8_t>("rsu-id", -1);
					confParam.setRsuId(id); 
				}else{
					id = section.get<int8_t>("car-id", -1);
					confParam.setCarId(id); 
		//			uint32_t intv = section.get<int32_t>("rsu-request-id-interval", 500);
		//			confParam.setRsuRequestIdInterval( ndn::time::milliseconds(intv) ); 
				}

			}else if (tn.first == "v2x-prefix"){
				confParam.setPrefixEvent( section.get<std::string>("auto-car-event") );
				confParam.setPrefixNoti( section.get<std::string>("noti") );
				confParam.setPrefixHorizon( section.get<std::string>("c-horizon") );
				
			}
		}

		spdlog::stdout_color_mt("dcn");

		std::cout << confParam << endl;

        NdnV2x ndnObu( face, confParam, log );

        face.getIoService().run();

    } catch(std::exception& e) {
        cout << "V2X-NDN -> exception: " << e.what() << "\n";
        return 1;
    }
    return 0;

}
