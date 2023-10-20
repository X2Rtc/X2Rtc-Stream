#include "RtmpMediaSource.h"
namespace mediakit {


void RtmpMediaSource::onWrite(RtmpPacket::Ptr pkt, bool ss) {};

/**
 * 获取当前时间戳
 */
uint32_t RtmpMediaSource::getTimeStamp(TrackType trackType) { return 0; };
};