#include "RTC/RTCP/FuzzerFeedbackPsFir.hpp"

void Fuzzer::RTC::RTCP::FeedbackPsFir::Fuzz(::RTC::RTCP::FeedbackPsFirPacket* packet)
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
		item->GetSequenceNumber();
	}
}
