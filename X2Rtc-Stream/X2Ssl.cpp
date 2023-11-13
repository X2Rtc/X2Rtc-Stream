#include "X2Ssl.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

static int gRtcPort = 0;
static httplib::SSLServer* gHttpRtcProxy = NULL;
static httplib::SSLServer* gHttpHtml = NULL;

int InitRtcProxy(int nSslPort, int nRtcPort)
{
	if (gHttpRtcProxy == NULL) {
		gRtcPort = nRtcPort;
		gHttpRtcProxy = new httplib::SSLServer("./Res/ssl/x2rtc.crt", "./Res/ssl/x2rtc.key");
		if (!gHttpRtcProxy->is_valid()) {
			return -1;
		}
		{
			gHttpRtcProxy->Post("/x2rtc/v1/pushstream", [&](const httplib::Request& req, httplib::Response& res) {//* Connection
				httplib::Client cli("127.0.0.1", gRtcPort);
				cli.set_connection_timeout(0, 500000); // 500 milliseconds
				cli.set_read_timeout(1, 0); // 1 seconds
				cli.set_write_timeout(1, 0); // 1 seconds
				auto cliRes = cli.Post("/x2rtc/v1/pushstream", req.body.c_str(), req.body.size(), "application/json");
				//res->status;
				//res->body;
				res.set_header("Server", " X2Rtc-Stream");
				res.set_header("Access-Control-Allow-Origin", "*");
				if (cliRes != nullptr) {
					res.set_content(cliRes->body.c_str(), "application/json");
				}
				else {
					res.set_content("{\"code\":0,\"msg\": \"OK\"}", "application/json");
				
				}
				});

			gHttpRtcProxy->Post("/x2rtc/v1/pullstream", [&](const httplib::Request& req, httplib::Response& res) {//* Connection
				httplib::Client cli("127.0.0.1", gRtcPort);
				cli.set_connection_timeout(0, 500000); // 500 milliseconds
				cli.set_read_timeout(1, 0); // 1 seconds
				cli.set_write_timeout(1, 0); // 1 seconds
				auto cliRes = cli.Post("/x2rtc/v1/pullstream", req.body.c_str(), req.body.size(), "application/json");
				//res->status;
				//res->body;
				res.set_header("Server", " X2Rtc-Stream");
				res.set_header("Access-Control-Allow-Origin", "*");
				if (cliRes != nullptr) {
					res.set_content(cliRes->body.c_str(), "application/json");
				}
				else {
					res.set_content("{\"code\":0,\"msg\": \"OK\"}", "application/json");

				}
				});

			gHttpRtcProxy->Post("/x2rtc/v1/play", [&](const httplib::Request& req, httplib::Response& res) {//* Connection
				httplib::Client cli("127.0.0.1", gRtcPort);
				cli.set_connection_timeout(0, 500000); // 500 milliseconds
				cli.set_read_timeout(1, 0); // 1 seconds
				cli.set_write_timeout(1, 0); // 1 seconds
				auto cliRes = cli.Post("/x2rtc/v1/play", req.body.c_str(), req.body.size(), "application/json");
				//res->status;
				//res->body;
				res.set_header("Server", " X2Rtc-Stream");
				res.set_header("Access-Control-Allow-Origin", "*");
				if (cliRes != nullptr) {
					res.set_content(cliRes->body.c_str(), "application/json");
				}
				else {
					res.set_content("{\"code\":0,\"msg\": \"OK\"}", "application/json");

				}
				});
		}
		if (!gHttpRtcProxy->bind_to_port("0.0.0.0", nSslPort)) {
			return -2;
		}
		std::thread(std::bind(&httplib::Server::listen_after_bind, gHttpRtcProxy)).detach();
	}
	return 0;
}
int DeInitRtcProxy()
{
	if (gHttpRtcProxy != NULL) {
		gHttpRtcProxy->stop();
		delete gHttpRtcProxy;
		gHttpRtcProxy = NULL;
	}
	return 0;
}


int InitHtmlDemos(int nPort, const char* strDir)
{
	if (gHttpHtml == NULL) {
		gHttpHtml = new httplib::SSLServer("./Res/ssl/x2rtc.crt", "./Res/ssl/x2rtc.key");
		if (!gHttpHtml->is_valid()) {
			return -1;
		}
		if (gHttpHtml->bind_to_port("0.0.0.0", nPort)) {
			// Mount / to ./html directory
			auto ret = gHttpHtml->set_mount_point("/", strDir);
			if (!ret) {
				// The specified base directory doesn't exist...
				return -2;
			}
			else {
				printf("[X2Rtc] Init ssl server OK!\r\n");
			}
			std::thread(std::bind(&httplib::Server::listen_after_bind, gHttpHtml)).detach();
		}
	}
	return 0;
}
int DeInitHtmlDemos()
{
	if (gHttpHtml != NULL) {
		gHttpHtml->stop();
		delete gHttpHtml;
		gHttpHtml = NULL;
	}
	return 0;
}

