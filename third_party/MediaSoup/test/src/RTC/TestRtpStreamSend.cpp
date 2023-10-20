#include "common.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStream.hpp"
#include "RTC/RtpStreamSend.hpp"
#include <catch2/catch.hpp>
#include <vector>

// #define PERFORMANCE_TEST 1

using namespace RTC;

static RtpPacket* CreateRtpPacket(uint8_t* buffer, uint16_t seq, uint32_t timestamp)
{
	auto* packet = RtpPacket::Parse(buffer, 1500);

	packet->SetSequenceNumber(seq);
	packet->SetTimestamp(timestamp);

	return packet;
}

static void SendRtpPacket(std::vector<std::pair<RtpStreamSend*, uint32_t>> streams, RtpPacket* packet)
{
	std::shared_ptr<RtpPacket> sharedPacket;

	for (auto& stream : streams)
	{
		packet->SetSsrc(stream.second);
		stream.first->ReceivePacket(packet, sharedPacket);
	}
}

static void CheckRtxPacket(RtpPacket* packet, uint16_t seq, uint32_t timestamp)
{
	REQUIRE(packet);
	REQUIRE(packet->GetSequenceNumber() == seq);
	REQUIRE(packet->GetTimestamp() == timestamp);
}

SCENARIO("NACK and RTP packets retransmission", "[rtp][rtcp][nack]")
{
	class TestRtpStreamListener : public RtpStreamSend::Listener
	{
	public:
		void OnRtpStreamScore(RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/) override
		{
		}

		void OnRtpStreamRetransmitRtpPacket(RtpStreamSend* /*rtpStream*/, RtpPacket* packet) override
		{
			this->retransmittedPackets.push_back(packet);
		}

	public:
		std::vector<RtpPacket*> retransmittedPackets;
	};

	// clang-format off
	uint8_t rtpBuffer1[] =
	{
		0b10000000, 0b01111011, 0b01010010, 0b00001110,
		0b01011011, 0b01101011, 0b11001010, 0b10110101,
		0, 0, 0, 2
	};
	// clang-format on

	uint8_t rtpBuffer2[1500];
	uint8_t rtpBuffer3[1500];
	uint8_t rtpBuffer4[1500];
	uint8_t rtpBuffer5[1500];

	std::memcpy(rtpBuffer2, rtpBuffer1, sizeof(rtpBuffer1));
	std::memcpy(rtpBuffer3, rtpBuffer1, sizeof(rtpBuffer1));
	std::memcpy(rtpBuffer4, rtpBuffer1, sizeof(rtpBuffer1));
	std::memcpy(rtpBuffer5, rtpBuffer1, sizeof(rtpBuffer1));

	SECTION("receive NACK and get retransmitted packets")
	{
		// packet1 [pt:123, seq:21006, timestamp:1533790901]
		auto packet1 = CreateRtpPacket(rtpBuffer1, 21006, 1533790901);
		// packet2 [pt:123, seq:21007, timestamp:1533790901]
		auto packet2 = CreateRtpPacket(rtpBuffer2, 21007, 1533790901);
		// packet3 [pt:123, seq:21008, timestamp:1533793871]
		auto packet3 = CreateRtpPacket(rtpBuffer3, 21008, 1533793871);
		// packet4 [pt:123, seq:21009, timestamp:1533793871]
		auto packet4 = CreateRtpPacket(rtpBuffer4, 21009, 1533793871);
		// packet5 [pt:123, seq:21010, timestamp:1533796931]
		auto packet5 = CreateRtpPacket(rtpBuffer5, 21010, 1533796931);

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.useNack       = true;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto* stream = new RtpStreamSend(&testRtpStreamListener, params, mid);

		// Receive all the packets (some of them not in order and/or duplicated).
		SendRtpPacket({ { stream, params.ssrc } }, packet1);
		SendRtpPacket({ { stream, params.ssrc } }, packet3);
		SendRtpPacket({ { stream, params.ssrc } }, packet2);
		SendRtpPacket({ { stream, params.ssrc } }, packet3);
		SendRtpPacket({ { stream, params.ssrc } }, packet4);
		SendRtpPacket({ { stream, params.ssrc } }, packet5);
		SendRtpPacket({ { stream, params.ssrc } }, packet5);

		// Create a NACK item that request for all the packets.
		RTCP::FeedbackRtpNackPacket nackPacket(0, params.ssrc);
		auto* nackItem = new RTCP::FeedbackRtpNackItem(21006, 0b0000000000001111);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 21006);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000001111);

		stream->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 5);

		auto rtxPacket1 = testRtpStreamListener.retransmittedPackets[0];
		auto rtxPacket2 = testRtpStreamListener.retransmittedPackets[1];
		auto rtxPacket3 = testRtpStreamListener.retransmittedPackets[2];
		auto rtxPacket4 = testRtpStreamListener.retransmittedPackets[3];
		auto rtxPacket5 = testRtpStreamListener.retransmittedPackets[4];

		testRtpStreamListener.retransmittedPackets.clear();

		CheckRtxPacket(rtxPacket1, packet1->GetSequenceNumber(), packet1->GetTimestamp());
		CheckRtxPacket(rtxPacket2, packet2->GetSequenceNumber(), packet2->GetTimestamp());
		CheckRtxPacket(rtxPacket3, packet3->GetSequenceNumber(), packet3->GetTimestamp());
		CheckRtxPacket(rtxPacket4, packet4->GetSequenceNumber(), packet4->GetTimestamp());
		CheckRtxPacket(rtxPacket5, packet5->GetSequenceNumber(), packet5->GetTimestamp());

		delete stream;
	}

	SECTION("receive NACK and get zero retransmitted packets if useNack is not set")
	{
		// packet1 [pt:123, seq:21006, timestamp:1533790901]
		auto packet1 = CreateRtpPacket(rtpBuffer1, 21006, 1533790901);
		// packet2 [pt:123, seq:21007, timestamp:1533790901]
		auto packet2 = CreateRtpPacket(rtpBuffer2, 21007, 1533790901);
		// packet3 [pt:123, seq:21008, timestamp:1533793871]
		auto packet3 = CreateRtpPacket(rtpBuffer3, 21008, 1533793871);
		// packet4 [pt:123, seq:21009, timestamp:1533793871]
		auto packet4 = CreateRtpPacket(rtpBuffer4, 21009, 1533793871);
		// packet5 [pt:123, seq:21010, timestamp:1533796931]
		auto packet5 = CreateRtpPacket(rtpBuffer5, 21010, 1533796931);

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.useNack       = false;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto* stream = new RtpStreamSend(&testRtpStreamListener, params, mid);

		// Receive all the packets (some of them not in order and/or duplicated).
		SendRtpPacket({ { stream, params.ssrc } }, packet1);
		SendRtpPacket({ { stream, params.ssrc } }, packet3);
		SendRtpPacket({ { stream, params.ssrc } }, packet2);
		SendRtpPacket({ { stream, params.ssrc } }, packet3);
		SendRtpPacket({ { stream, params.ssrc } }, packet4);
		SendRtpPacket({ { stream, params.ssrc } }, packet5);
		SendRtpPacket({ { stream, params.ssrc } }, packet5);

		// Create a NACK item that request for all the packets.
		RTCP::FeedbackRtpNackPacket nackPacket(0, params.ssrc);
		auto* nackItem = new RTCP::FeedbackRtpNackItem(21006, 0b0000000000001111);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 21006);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000001111);

		stream->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 0);

		testRtpStreamListener.retransmittedPackets.clear();

		delete stream;
	}

	SECTION("receive NACK and get zero retransmitted packets for audio")
	{
		// packet1 [pt:123, seq:21006, timestamp:1533790901]
		auto packet1 = CreateRtpPacket(rtpBuffer1, 21006, 1533790901);
		// packet2 [pt:123, seq:21007, timestamp:1533790901]
		auto packet2 = CreateRtpPacket(rtpBuffer2, 21007, 1533790901);
		// packet3 [pt:123, seq:21008, timestamp:1533793871]
		auto packet3 = CreateRtpPacket(rtpBuffer3, 21008, 1533793871);
		// packet4 [pt:123, seq:21009, timestamp:1533793871]
		auto packet4 = CreateRtpPacket(rtpBuffer4, 21009, 1533793871);
		// packet5 [pt:123, seq:21010, timestamp:1533796931]
		auto packet5 = CreateRtpPacket(rtpBuffer5, 21010, 1533796931);

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.useNack       = false;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::AUDIO;

		std::string mid;
		auto* stream = new RtpStreamSend(&testRtpStreamListener, params, mid);

		// Receive all the packets (some of them not in order and/or duplicated).
		SendRtpPacket({ { stream, params.ssrc } }, packet1);
		SendRtpPacket({ { stream, params.ssrc } }, packet3);
		SendRtpPacket({ { stream, params.ssrc } }, packet2);
		SendRtpPacket({ { stream, params.ssrc } }, packet3);
		SendRtpPacket({ { stream, params.ssrc } }, packet4);
		SendRtpPacket({ { stream, params.ssrc } }, packet5);
		SendRtpPacket({ { stream, params.ssrc } }, packet5);

		// Create a NACK item that request for all the packets.
		RTCP::FeedbackRtpNackPacket nackPacket(0, params.ssrc);
		auto* nackItem = new RTCP::FeedbackRtpNackItem(21006, 0b0000000000001111);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 21006);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000001111);

		stream->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener.retransmittedPackets.size() == 0);

		testRtpStreamListener.retransmittedPackets.clear();

		delete stream;
	}

	SECTION("receive NACK in different RtpStreamSend instances and get retransmitted packets")
	{
		// packet1 [pt:123, seq:21006, timestamp:1533790901]
		auto packet1 = CreateRtpPacket(rtpBuffer1, 21006, 1533790901);
		// packet2 [pt:123, seq:21007, timestamp:1533790901]
		auto packet2 = CreateRtpPacket(rtpBuffer2, 21007, 1533790901);

		// Create two RtpStreamSend instances.
		TestRtpStreamListener testRtpStreamListener1;
		TestRtpStreamListener testRtpStreamListener2;

		RtpStream::Params params1;

		params1.ssrc          = 1111;
		params1.clockRate     = 90000;
		params1.useNack       = true;
		params1.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto* stream1 = new RtpStreamSend(&testRtpStreamListener1, params1, mid);

		RtpStream::Params params2;

		params2.ssrc          = 2222;
		params2.clockRate     = 90000;
		params2.useNack       = true;
		params2.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		auto* stream2 = new RtpStreamSend(&testRtpStreamListener2, params2, mid);

		// Receive all the packets in both streams.
		SendRtpPacket({ { stream1, params1.ssrc }, { stream2, params2.ssrc } }, packet1);
		SendRtpPacket({ { stream1, params1.ssrc }, { stream2, params2.ssrc } }, packet2);

		// Create a NACK item that request for all the packets.
		RTCP::FeedbackRtpNackPacket nackPacket(0, params1.ssrc);
		auto* nackItem = new RTCP::FeedbackRtpNackItem(21006, 0b0000000000000001);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 21006);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000000001);

		// Process the NACK packet on stream1.
		stream1->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener1.retransmittedPackets.size() == 2);

		auto rtxPacket1 = testRtpStreamListener1.retransmittedPackets[0];
		auto rtxPacket2 = testRtpStreamListener1.retransmittedPackets[1];

		testRtpStreamListener1.retransmittedPackets.clear();

		CheckRtxPacket(rtxPacket1, packet1->GetSequenceNumber(), packet1->GetTimestamp());
		CheckRtxPacket(rtxPacket2, packet2->GetSequenceNumber(), packet2->GetTimestamp());

		// Process the NACK packet on stream2.
		stream2->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener2.retransmittedPackets.size() == 2);

		rtxPacket1 = testRtpStreamListener2.retransmittedPackets[0];
		rtxPacket2 = testRtpStreamListener2.retransmittedPackets[1];

		testRtpStreamListener2.retransmittedPackets.clear();

		CheckRtxPacket(rtxPacket1, packet1->GetSequenceNumber(), packet1->GetTimestamp());
		CheckRtxPacket(rtxPacket2, packet2->GetSequenceNumber(), packet2->GetTimestamp());

		delete stream1;
		delete stream2;
	}

	SECTION("packets get retransmitted as long as they don't exceed MaxRetransmissionDelay")
	{
		uint32_t clockRate = 90000;
		uint32_t firstTs   = 1533790901;
		uint32_t diffTs    = RtpStreamSend::MaxRetransmissionDelay * clockRate / 1000;
		uint32_t secondTs  = firstTs + diffTs;

		auto packet1 = CreateRtpPacket(rtpBuffer1, 21006, firstTs);
		auto packet2 = CreateRtpPacket(rtpBuffer2, 21007, secondTs - 1);

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener1;

		RtpStream::Params params1;

		params1.ssrc          = 1111;
		params1.clockRate     = clockRate;
		params1.useNack       = true;
		params1.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto* stream = new RtpStreamSend(&testRtpStreamListener1, params1, mid);

		// Receive all the packets.
		SendRtpPacket({ { stream, params1.ssrc } }, packet1);
		SendRtpPacket({ { stream, params1.ssrc } }, packet2);

		// Create a NACK item that request for all the packets.
		RTCP::FeedbackRtpNackPacket nackPacket(0, params1.ssrc);
		auto* nackItem = new RTCP::FeedbackRtpNackItem(21006, 0b0000000000000001);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 21006);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000000001);

		// Process the NACK packet on stream1.
		stream->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener1.retransmittedPackets.size() == 2);

		auto rtxPacket1 = testRtpStreamListener1.retransmittedPackets[0];
		auto rtxPacket2 = testRtpStreamListener1.retransmittedPackets[1];

		testRtpStreamListener1.retransmittedPackets.clear();

		CheckRtxPacket(rtxPacket1, packet1->GetSequenceNumber(), packet1->GetTimestamp());
		CheckRtxPacket(rtxPacket2, packet2->GetSequenceNumber(), packet2->GetTimestamp());

		delete stream;
	}

	SECTION("packets don't get retransmitted if MaxRetransmissionDelay is exceeded")
	{
		uint32_t clockRate = 90000;
		uint32_t firstTs   = 1533790901;
		uint32_t diffTs    = RtpStreamSend::MaxRetransmissionDelay * clockRate / 1000;
		uint32_t secondTs  = firstTs + diffTs;

		auto packet1 = CreateRtpPacket(rtpBuffer1, 21006, firstTs);
		auto packet2 = CreateRtpPacket(rtpBuffer2, 21007, secondTs);

		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener1;

		RtpStream::Params params1;

		params1.ssrc          = 1111;
		params1.clockRate     = clockRate;
		params1.useNack       = true;
		params1.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto* stream = new RtpStreamSend(&testRtpStreamListener1, params1, mid);

		// Receive all the packets.
		SendRtpPacket({ { stream, params1.ssrc } }, packet1);
		SendRtpPacket({ { stream, params1.ssrc } }, packet2);

		// Create a NACK item that request for all the packets.
		RTCP::FeedbackRtpNackPacket nackPacket(0, params1.ssrc);
		auto* nackItem = new RTCP::FeedbackRtpNackItem(21006, 0b0000000000000001);

		nackPacket.AddItem(nackItem);

		REQUIRE(nackItem->GetPacketId() == 21006);
		REQUIRE(nackItem->GetLostPacketBitmask() == 0b0000000000000001);

		// Process the NACK packet on stream1.
		stream->ReceiveNack(&nackPacket);

		REQUIRE(testRtpStreamListener1.retransmittedPackets.size() == 1);

		auto rtxPacket2 = testRtpStreamListener1.retransmittedPackets[0];

		testRtpStreamListener1.retransmittedPackets.clear();

		CheckRtxPacket(rtxPacket2, packet2->GetSequenceNumber(), packet2->GetTimestamp());

		delete stream;
	}

#ifdef PERFORMANCE_TEST
	SECTION("Performance")
	{
		// Create a RtpStreamSend instance.
		TestRtpStreamListener testRtpStreamListener;

		RtpStream::Params params;

		params.ssrc          = 1111;
		params.clockRate     = 90000;
		params.useNack       = true;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		std::string mid;
		auto* stream = new RtpStreamSend(&testRtpStreamListener, params, mid);

		size_t iterations = 10000000;

		auto start = std::chrono::system_clock::now();

		for (size_t i = 0; i < iterations; i++)
		{
			// Create packet.
			auto* packet = RtpPacket::Parse(rtpBuffer1, 1500);
			packet->SetSsrc(1111);

			std::shared_ptr<RtpPacket> sharedPacket(packet);

			stream->ReceivePacket(packet, sharedPacket);
		}

		std::chrono::duration<double> dur = std::chrono::system_clock::now() - start;
		std::cout << "nullptr && initialized shared_ptr: \t" << dur.count() << " seconds" << std::endl;

		delete stream;

		params.mimeType.type = RTC::RtpCodecMimeType::Type::AUDIO;
		stream               = new RtpStreamSend(&testRtpStreamListener, params, mid);

		start = std::chrono::system_clock::now();

		for (size_t i = 0; i < iterations; i++)
		{
			std::shared_ptr<RtpPacket> sharedPacket;

			// Create packet.
			auto* packet = RtpPacket::Parse(rtpBuffer1, 1500);
			packet->SetSsrc(1111);

			stream->ReceivePacket(packet, sharedPacket);
		}

		dur = std::chrono::system_clock::now() - start;
		std::cout << "raw && empty shared_ptr duration: \t" << dur.count() << " seconds" << std::endl;

		delete stream;
	}
#endif
}
