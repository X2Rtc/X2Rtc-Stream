#define MS_CLASS "RTC::RtpListener"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RtpListener.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/Producer.hpp"

namespace RTC
{
	/* Instance methods. */

	void RtpListener::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		jsonObject["ssrcTable"] = json::object();
		jsonObject["midTable"]  = json::object();
		jsonObject["ridTable"]  = json::object();

		auto jsonSsrcTableIt = jsonObject.find("ssrcTable");
		auto jsonMidTableIt  = jsonObject.find("midTable");
		auto jsonRidTableIt  = jsonObject.find("ridTable");

		// Add ssrcTable.
		for (const auto& kv : this->ssrcTable)
		{
			auto ssrc      = kv.first;
			auto* producer = kv.second;

			(*jsonSsrcTableIt)[std::to_string(ssrc)] = producer->id;
		}

		// Add midTable.
		for (const auto& kv : this->midTable)
		{
			const auto& mid = kv.first;
			auto* producer  = kv.second;

			(*jsonMidTableIt)[mid] = producer->id;
		}

		// Add ridTable.
		for (const auto& kv : this->ridTable)
		{
			const auto& rid = kv.first;
			auto* producer  = kv.second;

			(*jsonRidTableIt)[rid] = producer->id;
		}
	}

	void RtpListener::AddProducer(RTC::Producer* producer)
	{
		MS_TRACE();

		const auto& rtpParameters = producer->GetRtpParameters();

		// Add entries into the ssrcTable.
		for (const auto& encoding : rtpParameters.encodings)
		{
			uint32_t ssrc;

			// Check encoding.ssrc.
			ssrc = encoding.ssrc;

			if (ssrc != 0u)
			{
				if (this->ssrcTable.find(ssrc) == this->ssrcTable.end())
				{
					this->ssrcTable[ssrc] = producer;
				}
				else
				{
					RemoveProducer(producer);

					MS_THROW_ERROR("ssrc already exists in RTP listener [ssrc:%" PRIu32 "]", ssrc);
				}
			}

			// Check encoding.rtx.ssrc.
			ssrc = encoding.rtx.ssrc;

			if (ssrc != 0u)
			{
				if (this->ssrcTable.find(ssrc) == this->ssrcTable.end())
				{
					this->ssrcTable[ssrc] = producer;
				}
				else
				{
					RemoveProducer(producer);

					MS_THROW_ERROR("RTX ssrc already exists in RTP listener [ssrc:%" PRIu32 "]", ssrc);
				}
			}
		}

		// Add entries into midTable.
		if (!rtpParameters.mid.empty())
		{
			const auto& mid = rtpParameters.mid;

			if (this->midTable.find(mid) == this->midTable.end())
			{
				this->midTable[mid] = producer;
			}
			else
			{
				RemoveProducer(producer);

				MS_THROW_ERROR("MID already exists in RTP listener [mid:%s]", mid.c_str());
			}
		}

		// Add entries into ridTable.
		for (const auto& encoding : rtpParameters.encodings)
		{
			const auto& rid = encoding.rid;

			if (rid.empty())
				continue;

			if (this->ridTable.find(rid) == this->ridTable.end())
			{
				this->ridTable[rid] = producer;
			}
			// Just fail if no MID is given.
			else if (rtpParameters.mid.empty())
			{
				RemoveProducer(producer);

				MS_THROW_ERROR("RID already exists in RTP listener and no MID is given [rid:%s]", rid.c_str());
			}
		}
	}

	void RtpListener::RemoveProducer(RTC::Producer* producer)
	{
		MS_TRACE();

		// Remove from the listener tables all entries pointing to the Producer.

		for (auto it = this->ssrcTable.begin(); it != this->ssrcTable.end();)
		{
			if (it->second == producer)
				it = this->ssrcTable.erase(it);
			else
				++it;
		}

		for (auto it = this->midTable.begin(); it != this->midTable.end();)
		{
			if (it->second == producer)
				it = this->midTable.erase(it);
			else
				++it;
		}

		for (auto it = this->ridTable.begin(); it != this->ridTable.end();)
		{
			if (it->second == producer)
				it = this->ridTable.erase(it);
			else
				++it;
		}
	}

	RTC::Producer* RtpListener::GetProducer(const RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// First lookup into the SSRC table.
		{
			auto it = this->ssrcTable.find(packet->GetSsrc());

			if (it != this->ssrcTable.end())
			{
				auto* producer = it->second;

				return producer;
			}
		}

		// Otherwise lookup into the MID table.
		{
			std::string mid;

			if (packet->ReadMid(mid))
			{
				auto it = this->midTable.find(mid);

				if (it != this->midTable.end())
				{
					auto* producer = it->second;

					// Fill the ssrc table.
					// NOTE: We may be overriding an exiting SSRC here, but we don't care.
					this->ssrcTable[packet->GetSsrc()] = producer;

					return producer;
				}
			}
		}

		// Otherwise lookup into the RID table.
		{
			std::string rid;

			if (packet->ReadRid(rid))
			{
				auto it = this->ridTable.find(rid);

				if (it != this->ridTable.end())
				{
					auto* producer = it->second;

					// Fill the ssrc table.
					// NOTE: We may be overriding an exiting SSRC here, but we don't care.
					this->ssrcTable[packet->GetSsrc()] = producer;

					return producer;
				}
			}
		}

		return nullptr;
	}

	RTC::Producer* RtpListener::GetProducer(uint32_t ssrc) const
	{
		MS_TRACE();

		// Lookup into the SSRC table.
		auto it = this->ssrcTable.find(ssrc);

		if (it != this->ssrcTable.end())
		{
			auto* producer = it->second;

			return producer;
		}

		return nullptr;
	}
} // namespace RTC
