#include "X2ProtoHandler.h"

namespace x2rtc {
extern X2ProtoHandler* createX2ProtoRtmpHandler();
extern X2ProtoHandler* createX2ProtoMpegTsHandler();
extern X2ProtoHandler* createX2ProtoRtcHandler();
extern X2ProtoHandler* createX2ProtoRtpHandler();
extern X2ProtoHandler* createX2ProtoFlvHandler();
#ifdef ENABLE_X2_TALK
extern X2ProtoHandler* createX2ProtoTalkHandler();
#else
X2ProtoHandler* createX2ProtoTalkHandler()
{
	return NULL;
}
#endif
X2ProtoHandler* createX2ProtoHandler(X2ProtoType eType)
{
	X2ProtoHandler* x2ProtoHandler = NULL;
	if (eType == X2Proto_Rtmp) {
		x2ProtoHandler = createX2ProtoRtmpHandler();
	}
	else if (eType == X2Proto_MpegTS) {
		x2ProtoHandler = createX2ProtoMpegTsHandler();
	}
	else if (eType == X2Proto_Rtc) {
		x2ProtoHandler = createX2ProtoRtcHandler();
	}
	else if (eType == X2Proto_Rtp) {
		x2ProtoHandler = createX2ProtoRtpHandler();
	}
	else if (eType == X2Proto_Flv) {
		x2ProtoHandler = createX2ProtoFlvHandler();
	}
	else if (eType == X2Proto_Talk) {
		x2ProtoHandler = createX2ProtoTalkHandler();
	}

	if (x2ProtoHandler != NULL) {
		x2ProtoHandler->SetType(eType);
	}

	return x2ProtoHandler;
}

bool IsJson(const uint8_t* pData, int nLen) {
	if (pData != NULL && nLen > 0) {
		if (pData[0] == '{' && pData[nLen - 1] == '}') {
			return true;
		}
	}
	return false;
}

}	// namespace x2rtc