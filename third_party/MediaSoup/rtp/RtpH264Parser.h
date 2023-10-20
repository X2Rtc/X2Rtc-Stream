#ifndef ERIZO_SRC_ERIZO_RTP_RTPH264PARSER_H_
#define ERIZO_SRC_ERIZO_RTP_RTPH264PARSER_H_

//#include "./logger.h"
#include <memory>

const int16_t kNoPictureId = -1;
const int16_t kNoTl0PicIdx = -1;
const uint8_t kNoTemporalIdx = 0xFF;
const int kNoKeyIdx = -1;

namespace erizo {

	enum NaluType : uint8_t {
		kSlice = 1,
		kIdr = 5,
		kSei = 6,
		kSps = 7,
		kPps = 8,
		kAud = 9,
		kEndOfSequence = 10,
		kEndOfStream = 11,
		kFiller = 12,
		kStapA = 24,
		kFuA = 28
	};

	struct FrameMarking {
		bool start_of_frame;
		bool end_of_frame;
		bool independent_frame;
		bool discardable_frame;
		bool base_layer_sync;
		uint8_t temporal_id;
		uint8_t layer_id;
		uint8_t tl0_pic_idx;
	};

struct NaluInfo {
	uint8_t type;
	int sps_id;
	int pps_id;
};

const size_t kMaxNalusPerPacket = 10;

enum H264FrameTypes {
  kH264IFrame,  // key frame
  kH264PFrame   // Delta frame
};

enum NALTypes {
    single,
    fragmented,
    aggregated,
};

struct RTPPayloadH264 {
  static constexpr unsigned char start_sequence[] = { 0, 0, 0, 1 };
  H264FrameTypes frameType = kH264PFrame;
  NALTypes nal_type = single;
  const unsigned char* data;
  unsigned int dataLength = 0;
  unsigned char fragment_nal_header;
  int fragment_nal_header_len = 0;
  unsigned char start_bit = 0;
  unsigned char end_bit = 0;
  std::unique_ptr<unsigned char[]> unpacked_data;
  unsigned unpacked_data_len = 0;
  int width = 0;
  int height = 0;
  NaluInfo nalus[kMaxNalusPerPacket];
  size_t nalus_length = 0;
  char video_payload[1500];
  int video_payload_size = 0;

  FrameMarking frame_marking = { false, false, false, false, false, 0xFF, 0, 0 };
};

class RtpH264Parser {
  //DECLARE_LOGGER();
 public:
  RtpH264Parser();
  virtual ~RtpH264Parser();
  erizo::RTPPayloadH264* parseH264(unsigned char* data, int datalength);
  bool parseH264(erizo::RTPPayloadH264* h264, unsigned char* data, int datalength);
 private:
  int parse_packet_fu_a(RTPPayloadH264* h264, unsigned char* buf, int len) const;
  int parse_aggregated_packet(RTPPayloadH264* h264, unsigned char* buf, int len) const;
};
}  // namespace erizo
#endif  // ERIZO_SRC_ERIZO_RTP_RTPH264PARSER_H_
