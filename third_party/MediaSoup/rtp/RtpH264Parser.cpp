#include "rtp/RtpH264Parser.h"

//#include <arpa/inet.h>

#include <cstddef>
#include <cstdio>
#include <string>
#include <iostream>
#include <cstdint>
#include <cstring>
//#include "libavutil/intreadwrite.h"
#include "intreadwrite.h"

// Bit masks for FU (A and B) indicators.
enum NalDefs : uint8_t { kFBit = 0x80, kNriMask = 0x60, kTypeMask = 0x1F };

// Bit masks for FU (A and B) headers.
enum FuDefs : uint8_t { kSBit = 0x80, kEBit = 0x40, kRBit = 0x20 };

namespace erizo {

constexpr unsigned char RTPPayloadH264::start_sequence[];

//DEFINE_LOGGER(RtpH264Parser, "rtp.RtpH264Parser");

RtpH264Parser::RtpH264Parser() {
}

RtpH264Parser::~RtpH264Parser() {
}

//
// H264 format:
//
// +---------------+
// |0|1|2|3|4|5|6|7|
// +-+-+-+-+-+-+-+-+
// |F|NRI|  Type   |
// +---------------+

RTPPayloadH264* RtpH264Parser::parseH264(unsigned char* buf, int len) {
  RTPPayloadH264* h264 = new RTPPayloadH264;
  if (!len) {
    //ELOG_ERROR("Empty H.264 RTP packet");
    return h264;
  }

  parseH264(h264, buf, len);

  return h264;
}
bool RtpH264Parser::parseH264(erizo::RTPPayloadH264* h264, unsigned char* buf, int len)
{
	uint8_t nal;
	uint8_t type;

	if (!len) {
		//ELOG_ERROR("Empty H.264 RTP packet");
		return false;
	}

	nal = buf[0];
	type = nal & 0x1f;

	/* Simplify the case (these are all the NAL types used internally by the H.264 codec). */
	if (type >= 1 && type <= 23 && type != 5) {
		type = 1;
	}

	switch (type) {
	case 5:             // keyframe
		h264->frameType = kH264IFrame;
		// Falls through
	case 0:          // undefined, but pass them through
	case 1:
		h264->nal_type = single;
		h264->start_bit = true;
		h264->data = buf;
		h264->dataLength = len;
		{
			NaluInfo nalu;
			nalu.type = type;
			nalu.sps_id = -1;
			nalu.pps_id = -1;
			h264->nalus[h264->nalus_length] = nalu;
			h264->nalus_length = 1;
		}
		break;
	case 24:           // STAP-A (one packet, multiple nals)	//STAP-A的作用是可以把连个nalu单元封装在一个RTP包里面进行传输，注意的是-A的格式都是不允许跨帧的，也就是两个nalu单元的时间戳必须是相同的。常见的场景是sps和pps两个小包被合并封装。
	  // Consume the STAP-A NAL
		buf++;
		len--;
		parse_aggregated_packet(h264, buf, len);
		break;
	case 25:           // STAP-B
	case 26:           // MTAP-16
	case 27:           // MTAP-24
	case 29:           // FU-B
	  //ELOG_ERROR("Unhandled H264 NAL unit type (%d)", type);
		break;
	case 28:           // FU-A (fragmented nal)	//FU-A的作用是把一个原始的大的nalu切成多个数据包进行传输，主要场景在一个slice比较大的情况。FU-A会比较特殊，有FU-A起始包，FU-A包（如果只切两个包可能没有）和FU-A结束包组成。
		parse_packet_fu_a(h264, buf, len);
		break;
	case 30:           // undefined
	case 31:           // undefined
	default:
		//ELOG_ERROR("Undefined H264 NAL unit type (%d)", type);
		break;
	}
	return true;
}
int RtpH264Parser::parse_packet_fu_a(RTPPayloadH264* h264, unsigned char* buf, int len) const {
  uint8_t fu_indicator, fu_header, start_bit, nal_type, nal, end_bit;

  if (len < 3) {
    //ELOG_ERROR("Too short data for FU-A H.264 RTP packet");
    return -1;
  }

  fu_indicator = buf[0];
  fu_header  = buf[1];
  start_bit  = fu_header >> 7;
  end_bit    = fu_header & 0x40;
  nal_type   = fu_header & 0x1f;
  nal      = (fu_indicator & 0xe0) | nal_type;

  bool first_fragment = (buf[1] & kSBit) > 0;
  uint8_t original_nal_type = buf[1] & kTypeMask;
  NaluInfo nalu;
  nalu.type = original_nal_type;
  nalu.sps_id = -1;
  nalu.pps_id = -1;
  // skip the fu_indicator and fu_header
  buf += 2;
  len -= 2;

  if (nal_type == 5 && start_bit == 1) {
    h264->frameType = kH264IFrame;
  }

  h264->nal_type = fragmented;
  h264->start_bit = start_bit;
  h264->end_bit = end_bit;
  h264->fragment_nal_header = nal;
  h264->fragment_nal_header_len = 1;
  h264->data = buf;
  h264->dataLength = len;
  if (first_fragment) {
	  h264->nalus[h264->nalus_length] = nalu;
	  h264->nalus_length = 1;
  }
  return 0;
}

int RtpH264Parser::parse_aggregated_packet(RTPPayloadH264* h264, unsigned char* buf, int len) const {
  h264->nal_type = aggregated;
  h264->start_bit = true;
  unsigned char* dst = nullptr;
  int pass     = 0;
  int total_length = 0;

  // first we are going to figure out the total size
  for (pass = 0; pass < 2; pass++) {
    const uint8_t *src = buf;
    int src_len    = len;

    while (src_len > 2) {
      uint16_t nal_size = AV_RB16(src);

      // consume the length of the aggregate
      src   += 2;
      src_len -= 2;

      if (nal_size <= src_len) {
        if (pass == 0) {
          // counting
          total_length += sizeof(RTPPayloadH264::start_sequence) + nal_size;
        } else {
          // copying
          std::memcpy(dst, RTPPayloadH264::start_sequence, sizeof(RTPPayloadH264::start_sequence));
          dst += sizeof(RTPPayloadH264::start_sequence);
          std::memcpy(dst, src, nal_size);
          dst += nal_size;
          uint8_t nal_type = src[0] & 0x1f;
          if (nal_type == 5) {
            h264->frameType = kH264IFrame;
          }
		  NaluInfo nalu;
		  nalu.type = src[0] & kTypeMask;
		  nalu.sps_id = -1;
		  nalu.pps_id = -1;

		  if (h264->nalus_length == kMaxNalusPerPacket) {
			  /*RTC_LOG(LS_WARNING)
				  << "Received packet containing more than " << kMaxNalusPerPacket
				  << " NAL units. Will not keep track sps and pps ids for all of them.";*/
		  }
		  else {
			  h264->nalus[h264->nalus_length++] = nalu;
		  }
        }
      } else {
        //ELOG_ERROR("NAL size exceeds length: %d %d\n", nal_size, src_len);
        return -1;
      }

      // eat what we handled
      src   += nal_size;
      src_len -= nal_size;
    }

    if (pass == 0) {
      /* now we know the total size of the packet (with the start sequences added) */
      h264->unpacked_data_len = total_length;
      h264->unpacked_data = std::unique_ptr<unsigned char[]>(new unsigned char[total_length]);
      dst = &h264->unpacked_data[0];
    }
  }
  return 0;
}

}  // namespace erizo
