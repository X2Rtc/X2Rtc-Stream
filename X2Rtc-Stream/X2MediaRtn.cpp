#include "X2MediaRtn.h"
#include "Libuv.h"

namespace x2rtc {
static uv_lib_t* gUvLib = NULL;

typedef int (*funcRtn_Set_cbX2GtRtnReportEvent)(cbX2GtRtnReportEvent func);
typedef int (*funcRtn_Set_cbX2GtRtnPublishEvent)(cbX2GtRtnPublishEvent func);
typedef int (*funcRtn_Set_cbX2GtRtnRecvStream)(cbX2GtRtnRecvStream func);

typedef int(*funcRtn_Init)(const char* strConf);
typedef int(*funcRtn_DeInit)();
typedef int(*funcRtn_UpdateGlobalPath)(const char* strGlobalPath);
typedef int(*funcRtn_PublishStream)(const char* strAppId, const char* strStreamId, const char* strResSId, void* userData);
typedef int(*funcRtn_UnPublishStream)(const char* strAppId, const char* strStreamId);
typedef int(*funcRtn_SendPublishStream)(const char* strAppId, const char* strStreamId, X2MediaData* pMediaData);
typedef int(*funcRtn_SubscribeStream)(const char* strAppId, const char* strStreamId, void* userData);
typedef int(*funcRtn_UnSubscribeStream)(const char* strAppId, const char* strStreamId);
typedef int(*funcRtn_SubscribeAudioCodec)(const char* strAppId, const char* strStreamId, X2GtRtnCodec eCType, bool bEnable);

void Rtn_cbX2GtRtnReportEvent(X2GtRtnRptEvent eEventType, X2GtRtnCode eCode, const char* strInfo)
{

}
void Rtn_cbX2GtRtnPublishEvent(void* userData, X2GtRtnPubEvent eEventType, X2GtRtnPubState eState, const char* strInfo)
{
	X2MediaRtnPub* x2MediaRtn = (X2MediaRtnPub*)userData;
	x2MediaRtn->OnX2GtRtnPublishEvent(eEventType, eState, strInfo);
}

void Rtn_cbX2GtRtnRecvStream(void* userData, X2MediaData* pMediaData)
{
	X2MediaRtnPub* x2MediaRtn = (X2MediaRtnPub*)userData;
	x2MediaRtn->OnX2GtRtnRecvStream(pMediaData);
}

bool X2MediaRtn::InitRtn(const char* strConf)
{
	if (gUvLib == NULL) {
#ifdef _WIN32
		const char* libPath = "./lib/X2Gt-Rtn.dll";
#else
		const char* libPath = "./lib/libX2Gt-Rtn.so";
#endif
		gUvLib = new uv_lib_t();
		if (uv_dlopen(libPath, gUvLib) != 0) {
			uv_dlclose(gUvLib);
			delete gUvLib;
			gUvLib = NULL;
			return false;
		}
		void* funcRtn = NULL;
		if (uv_dlsym(gUvLib, "X2GtRtn_Init", &funcRtn) != 0 || funcRtn == NULL) {
			uv_dlclose(gUvLib);
			delete gUvLib;
			gUvLib = NULL;
			return false;
		}
		int ret = ((funcRtn_Init)funcRtn)(strConf);
		if (ret != 0) {
			uv_dlclose(gUvLib);
			delete gUvLib;
			gUvLib = NULL;
			return false;
		}

		{
			void* funcRtn = NULL;
			if (uv_dlsym(gUvLib, "X2GtRtn_Set_cbX2GtRtnReportEvent", &funcRtn) == 0 && funcRtn != NULL) {
				((funcRtn_Set_cbX2GtRtnReportEvent)funcRtn)(Rtn_cbX2GtRtnReportEvent);
			}
		}
		{
			void* funcRtn = NULL;
			if (uv_dlsym(gUvLib, "X2GtRtn_Set_cbX2GtRtnPublishEvent", &funcRtn) == 0 && funcRtn != NULL) {
				((funcRtn_Set_cbX2GtRtnPublishEvent)funcRtn)(Rtn_cbX2GtRtnPublishEvent);
			}
		}
		{
			void* funcRtn = NULL;
			if (uv_dlsym(gUvLib, "X2GtRtn_Set_cbX2GtRtnRecvStream", &funcRtn) == 0 && funcRtn != NULL) {
				((funcRtn_Set_cbX2GtRtnRecvStream)funcRtn)(Rtn_cbX2GtRtnRecvStream);
			}
		}
	}
	return true;
}
void X2MediaRtn::DeInitRtn()
{
	if (gUvLib != NULL) {
		void* funcRtn = NULL;
		if (uv_dlsym(gUvLib, "X2GtRtn_DeInit", &funcRtn) == 0 && funcRtn != NULL) {
			((funcRtn_DeInit)funcRtn)();
		}
		uv_dlclose(gUvLib);
		delete gUvLib;
		gUvLib = NULL;
	}
}


X2MediaRtnPub::X2MediaRtnPub(void)
	: b_publish_ok_(false)
{

}
X2MediaRtnPub::~X2MediaRtnPub(void)
{

}

void X2MediaRtnPub::RtnPublish(const char* strAppId, const char* strStreamId)
{
	int ret = -1;
	if (gUvLib != NULL) {
		void* funcRtn = NULL;
		if (uv_dlsym(gUvLib, "X2GtRtn_PublishStream", &funcRtn) == 0 && funcRtn != NULL) {
			ret = ((funcRtn_PublishStream)funcRtn)(strAppId, strStreamId, "123", this);
		}
	}
	if (ret == 0) {
		b_publish_ok_ = true;
		str_app_id_ = strAppId;
		str_stream_id_ = strStreamId;
	}
}
void X2MediaRtnPub::RtnUnPublish()
{
	if (gUvLib != NULL) {
		void* funcRtn = NULL;
		if (b_publish_ok_) {
			b_publish_ok_ = false;
			if (uv_dlsym(gUvLib, "X2GtRtn_UnPublishStream", &funcRtn) == 0 && funcRtn != NULL) {
				((funcRtn_UnPublishStream)funcRtn)(str_app_id_.c_str(), str_stream_id_.c_str());
			}
		}
	}
}

void X2MediaRtnPub::OnX2GtRtnPublishEvent(X2GtRtnPubEvent eEventType, X2GtRtnPubState eState, const char* strInfo)
{
	if (eEventType == X2GRPubEvent_OK) {
		OnX2GtRtnPublishOK();
	}
	else if (eEventType == X2GRPubEvent_Failed) {
		b_publish_ok_ = false;
		OnX2GtRtnPublishForceClosed(-1);
	}
	else if (eEventType == X2GRPubEvent_Closed) {
		b_publish_ok_ = false;
		OnX2GtRtnPublishForceClosed(-2);
	}
}
void X2MediaRtnPub::OnX2GtRtnRecvStream(X2MediaData* pMediaData)
{

}


X2MediaRtnSub::X2MediaRtnSub(void)
{

}
X2MediaRtnSub::~X2MediaRtnSub(void)
{

}

void X2MediaRtnSub::RtnSubscribe(const char* strAppId, const char* strStreamId)
{

}
void X2MediaRtnSub::RtnUnSubscribe()
{

}

}	// namespace x2rtc