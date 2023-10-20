#include "X2NetProcess.h"

namespace x2rtc {
	extern X2NetProcess* createX2NetRtmpProcess();
	extern X2NetProcess* createX2NetSrtProcess();
	extern X2NetProcess* createX2NetWebRtcProcess();
	extern X2NetProcess* createX2NetRtpProcess();
	extern X2NetProcess* createX2NetHttpProcess();
#ifdef ENABLE_X2_TALK
	extern X2NetProcess* createX2NetTalkProcess();
#else
	X2NetProcess* createX2NetTalkProcess() {
		return NULL;
	}
#endif

	X2NetProcess* createX2NetProcess(X2ProcessType eType)
	{
		X2NetProcess* x2NetProcess = NULL;
		switch (eType) {
		case X2Process_Rtmp: {
			x2NetProcess = createX2NetRtmpProcess();
		}break;
		case X2Process_Srt: {
			x2NetProcess = createX2NetSrtProcess();
		}break;
		case X2Process_Rtc: {
			x2NetProcess = createX2NetWebRtcProcess();
		}break;
		case X2Process_Rtp: {
			x2NetProcess = createX2NetRtpProcess();
		}break;
		case X2Process_Http: {
			x2NetProcess = createX2NetHttpProcess();
		}break;
		case X2Process_Talk: {
			x2NetProcess = createX2NetTalkProcess();
		}break;
		}

		return x2NetProcess;
	}

	const char* getX2NetTypeName(X2NetType eType) {
		switch (eType) {
#define X2T(name, value, str) case name : return str;
			X2NetType_MAP(X2T)
#undef X2T
		default: return "invalid";
		}
	}


	const char* getX2ProcessTypeName(X2ProcessType eType) {
		switch (eType) {
#define X2T(name, value, str) case name : return str;
			X2ProcessType_MAP(X2T)
#undef X2T
		default: return "invalid";
		}
	}

}	// namespace x2rtc