#include "X2ProtoFlvHandler.h"
#include "absl/memory/memory.h"

namespace x2rtc {

X2ProtoHandler* createX2ProtoFlvHandler()
{
	return new X2ProtoFlvHandler();
}
X2ProtoFlvHandler::X2ProtoFlvHandler(void)
{
	x2http_session_ = absl::make_unique<X2HttpSession>();
	x2http_session_->SetListener(this);
}
X2ProtoFlvHandler::~X2ProtoFlvHandler(void)
{
	x2http_session_ = NULL;
}

//* For X2ProtoHandler
void X2ProtoFlvHandler::MuxData(int codec, const uint8_t* data, size_t bytes, int64_t dts, int64_t pts, uint16_t audSeqn)
{
	if (x2http_session_ != NULL) {
		//printf("Data type: %d len: %d pts: %lld\r\n", codec, bytes, timestamp);
		if (codec == X2Codec_AAC) {
			x2http_session_->SendAacAudio(data, bytes, dts, pts);
		}
		else if (codec == X2Codec_H264) {
			x2http_session_->SendH264Video(data, bytes, dts, pts);
		}
		else if (codec == X2Codec_H265) {
			x2http_session_->SendH265Video(data, bytes, dts, pts);
		}
	}
}
void X2ProtoFlvHandler::RecvDataFromNetwork(int nDataType, const uint8_t* pData, size_t nLen)
{
	//printf("X2ProtoFlvHandler recv: %.*s\r\n", nLen, pData);
	try {
		// 可能会抛出std::runtime_error异常的代码
		if (x2http_session_ != NULL) {
			x2http_session_->RecvData((char*)pData, nLen);
		}
	}
	catch (...) {
		// 处理std::runtime_error异常的代码
		if (x2http_session_ != NULL) {
			x2http_session_->Close();
			x2http_session_ = NULL;
		}
		if (cb_listener_ != NULL) {
			cb_listener_->OnX2ProtoHandlerClose();
		}
	}
}
void X2ProtoFlvHandler::Close()
{
	if (x2http_session_ != NULL) {
		x2http_session_->stop();
		x2http_session_->Close();
		x2http_session_ = NULL;
	}
}

//* For X2HttpSession::Listener
void X2ProtoFlvHandler::OnPlay(const char* app, const char* stream, const char* args)
{
	if (cb_listener_ != NULL) {
		cb_listener_->OnX2ProtoHandlerPlay(app, stream, "", args);

		cb_listener_->OnX2ProtoHandlerSetPlayCodecType(X2Codec_AAC, X2Codec_None);
	}
	if (x2http_session_ != NULL) {
		x2http_session_->start();
	}
}
void X2ProtoFlvHandler::OnClose()
{
	if (cb_listener_ != NULL) {
		cb_listener_->OnX2ProtoHandlerClose();
	}
}
void X2ProtoFlvHandler::OnSendData(const char* pData, int nLen)
{
	if (cb_listener_ != NULL) {
		cb_listener_->OnX2ProtoHandlerSendData(NULL, 0, (uint8_t*)pData, nLen);
	}
}
void X2ProtoFlvHandler::OnSendData(const char* pHdr, int nHdrLen, const char* pData, int nLen)
{
	if (cb_listener_ != NULL) {
		cb_listener_->OnX2ProtoHandlerSendData((uint8_t*)pHdr, nHdrLen, (uint8_t*)pData, nLen);
	}
}

}	// namespace x2rtc