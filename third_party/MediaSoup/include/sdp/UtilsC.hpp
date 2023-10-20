#ifndef MSC_SDP_UTILS_HPP
#define MSC_SDP_UTILS_HPP

#include <json.hpp>
#include <sdptransform.hpp>
//#include <api/media_stream_interface.h>
#include <string>
#include <ctime>   // generator seed
#include <random>  // generators header

namespace mediasoupclient
{
	template<typename T>
	T getRandomInteger(T min, T max);

	std::vector<std::string> split(const std::string& s, char delimiter);
	std::string join(const std::vector<std::string>& v, char delimiter);
	std::string join(const std::vector<uint32_t>& v, char delimiter);

	template<typename T>
	inline T getRandomInteger(T min, T max)
	{
		// Seed with time.
		static unsigned int seed = time(nullptr);

		// Engine based on the Mersenne Twister 19937 (64 bits).
		static std::mt19937_64 rng(seed);

		// Uniform distribution for integers in the [min, max) range.
		std::uniform_int_distribution<T> dis(min, max);

		return dis(rng);
	}

	namespace Sdp
	{
		namespace Utils
		{
			json extractRtpCapabilities(const json& sdpObject);
			std::string extractTrackID(const json& sdpObject);
			json extractDtlsParameters(const json& sdpObject);
			void fillRtpParametersForTrack(json& rtpParameters, const json& sdpObject, const std::string& mid);
			void addLegacySimulcast(json& offerMediaObject, uint8_t numStreams);
			std::string getCname(const json& offerMediaObject);
			json getRtpEncodings(const json& offerMediaObject);
			void applyCodecParameters(const json& offerRtpParameters, json& answerMediaObject);
		} // namespace Utils
	}   // namespace Sdp
} // namespace mediasoupclient

#endif
