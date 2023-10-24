#include <stdio.h>
#include "X2Proxy.h"
#include "XUtil.h"
#include "Win32SocketInit.h"
#include "X2RtcLog.h"
#include "X2RtcCheck.h"
#include "X2SvrTmplate.h"
#include "libflv/rtc-check.h"
#include "X2RtcThreadPool.h"
#include "X2MediaDispatch.h"
#include "X2MediaRtn.h"
#include "X2Ssl.h"


void X2CheckPrintf(const char* fmt, ...)
{
	char buffer[2048];
	va_list argptr;
	va_start(argptr, fmt);
	int ret = vsnprintf(buffer, 2047, fmt, argptr);
	va_end(argptr);

	X2RtcLog(CRIT, buffer);
}
x2rtc_check funcX2RtcCheck = X2CheckPrintf;
rtc_flv_check funcRtcFlvCheck = X2CheckPrintf;
std::string gStrExtenIp;

static const char kVersion[] = "1.0.2_20220914_01";
void printInfo() {
	std::cout << "##############################################" << std::endl;
	std::cout << "#    X2Rtc: Stream Server (" << kVersion << ")" << std::endl;
	std::cout << "# " << std::endl;
	std::cout << "#    Authed by X2Rtc-Team" << std::endl;
	std::cout << "#    Website: https://www.x2rtc.com" << std::endl;
	std::cout << "##############################################" << std::endl;
	std::cout << "" << std::endl;
}

int main(int argc, char*argv[])
{
	X2SVR_INIT;
	X2SIGNAL_PIPE;

	x2rtc::WinsockInitializer winSockInitializer;

	X2RtcThreadPool::Inst().Init(2);
	X2MediaDispatch::Inst();

	gStrExtenIp = conf.GetValue("rtc", "ip");

	x2rtc::X2Proxy* x2Proxy = x2rtc::createX2Proxy();
	x2Proxy->ImportX2NetProcess(x2rtc::X2ProcessType::X2Process_Rtmp, conf.GetIntVal("mod", "rtmp", 1935));
	x2Proxy->ImportX2NetProcess(x2rtc::X2ProcessType::X2Process_Srt, conf.GetIntVal("mod", "srt", 8000));
	x2Proxy->ImportX2NetProcess(x2rtc::X2ProcessType::X2Process_Rtc, conf.GetIntVal("mod", "rtc", 10010));
	x2Proxy->ImportX2NetProcess(x2rtc::X2ProcessType::X2Process_Rtp, conf.GetIntVal("mod", "rtp", 1086));
	x2Proxy->ImportX2NetProcess(x2rtc::X2ProcessType::X2Process_Http, conf.GetIntVal("mod", "http", 1080));
	x2Proxy->ImportX2NetProcess(x2rtc::X2ProcessType::X2Process_Talk, conf.GetIntVal("mod", "talk", 1088));

	int nRtcPort = conf.GetIntVal("mod", "rtc", 10010);
	int nRtcSslPort = conf.GetIntVal("mod", "rtc_ssl", 10011);
	if (nRtcSslPort > 0 && nRtcPort > 0) {
		InitRtcProxy(nRtcSslPort, nRtcPort);
	}
	
	int nHtmlPort = conf.GetIntVal("html", "port", 0);
	if (nHtmlPort > 0) {
		const std::string& strDir = conf.GetValue("html", "dir");
		InitHtmlDemos(nHtmlPort, strDir.c_str());
	}

	std::string strRegIp = conf.GetValue("reg", "ip");
	int nRegPort = conf.GetIntVal("reg", "port");
	x2Proxy->StartTask(strRegIp.c_str(), nRegPort);

	X2MediaRtn::InitRtn("{\"License\":\"oiri3oufjo8r42uybiuayi\",\"Oem\":\"anyrtc\", \"Region\":\"CN_HD\", \"RtnType\":7, \"NodeId\":\"SH_121\", \"RtnUrl\":\"ws://192.168.1.130:8099\"}");

	while (1) {
		if (x2Proxy->RunOnce() != 0) {
			break;
		}

		X2MediaDispatch::Inst().RunOnce();
		X2RtcThreadPool::Inst().RunOnce();
		XSleep(1);
	}

	x2Proxy->StopTask();
	delete x2Proxy;
	x2Proxy = NULL;

	DeInitRtcProxy();
	DeInitHtmlDemos();

	X2RtcThreadPool::Inst().DeInit();

	return 0;
}
