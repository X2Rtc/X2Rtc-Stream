#ifndef SDP_SCALABILITY_MODE_H
#define SDP_SCALABILITY_MODE_H

#include <json.hpp>
#include <string>

namespace SdpParse
{
	nlohmann::json parseScalabilityMode(const std::string& scalabilityMode);
} // namespace SdpParse

#endif
