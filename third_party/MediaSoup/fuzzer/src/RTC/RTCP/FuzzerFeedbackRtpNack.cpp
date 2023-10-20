#include "RTC/RTCP/FuzzerFeedbackRtpNack.hpp"

void Fuzzer::RTC::RTCP::FeedbackRtpNack::Fuzz(::RTC::RTCP::FeedbackRtpNackPacket* packet)
{
	// packet->Dump();
	packet->Serialize(::RTC::RTCP::Buffer);
	packet->GetCount();
	packet->GetSize();

	// TODO.
	// AddItem(Item* item);

	auto it = packet->Begin();
	for (; it != packet->End(); ++it)
	{
		auto& item = (*it);

		// item->Dump();
		item->Serialize(::RTC::RTCP::Buffer);
		item->GetSize();
		item->GetPacketId();
		item->GetLostPacketBitmask();
	}
}
