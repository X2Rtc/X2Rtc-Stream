#include "RTC/RTCP/FuzzerFeedbackPsLei.hpp"

void Fuzzer::RTC::RTCP::FeedbackPsLei::Fuzz(::RTC::RTCP::FeedbackPsLeiPacket* packet)
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
		item->GetSsrc();
	}
}
