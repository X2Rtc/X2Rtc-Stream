#include "RtmpMediaSource.h"
namespace mediakit {


void RtmpMediaSource::onWrite(RtmpPacket::Ptr pkt, bool ss) {};

/**
 * ��ȡ��ǰʱ���
 */
uint32_t RtmpMediaSource::getTimeStamp(TrackType trackType) { return 0; };
};