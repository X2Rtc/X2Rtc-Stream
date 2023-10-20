/*
*  Copyright (c) 2022 The X2RTC project authors. All Rights Reserved.
*
*  Please visit https://www.x2rtc.com for detail.
*
*  Author: Eric(eric@x2rtc.com)
*  Twitter: @X2rtc_cloud
*
* The GNU General Public License is a free, copyleft license for
* software and other kinds of works.
*
* The licenses for most software and other practical works are designed
* to take away your freedom to share and change the works.  By contrast,
* the GNU General Public License is intended to guarantee your freedom to
* share and change all versions of a program--to make sure it remains free
* software for all its users.  We, the Free Software Foundation, use the
* GNU General Public License for most of our software; it applies also to
* any other work released this way by its authors.  You can apply it to
* your programs, too.
* See the GNU LICENSE file for more info.
*/
#ifndef __X2_NET_CONNECTION_H__ 
#define __X2_NET_CONNECTION_H__
#include <stdint.h>
#include <string.h>
#include <string>
#include <list>
#ifndef WIN32
#include <sys/socket.h>
#endif
#include "X2RtcRef.h"
#include "jmutexautolock.h"
#include "RefCountedObject.h"

namespace x2rtc {
#define X2NetType_MAP(X2T) \
	X2T(X2Net_Udp, 0, "UDP")			\
	X2T(X2Net_Tcp, 1, "TCP")			\
	X2T(X2Net_Rtmp, 2, "RTMP")			\
	X2T(X2Net_Srt, 3, "SRT")			\
	X2T(X2Net_Quic, 4, "QUIC")			\
	X2T(X2Net_Rtc, 5, "RTC")			\
	X2T(X2Net_Rtp, 6, "RTP")			\
	X2T(X2Net_Http, 7, "HTTP")			\
	X2T(X2Net_Talk, 8, "TALK")


enum X2NetType
{
	X2Net_Invalid = -1,
#define X2T(name,  value, str) name = value,
	X2NetType_MAP(X2T)
#undef X2T
	X2Net_Max,
};

const char* getX2NetTypeName(X2NetType eType);

struct X2NetData {
	X2NetData(): pHdr(NULL), nHdrLen(0), nDataType(0), pData(NULL), nLen(0), bSendOnce(false), recvTime(0){
	}
	virtual ~X2NetData(void) {
		delete[] pHdr;
		delete[] pData;
	}

	bool IsJson() {
		if (pData != NULL && nLen > 0) {
			if (pData[0] == '{' && pData[nLen - 1] == '}') {
				return true;
			}
		}
		return false;
	}

	void SetHdr(const uint8_t* data, int len) {
		if (len <= 0)
			return;
		nHdrLen = len;
		if (pHdr != NULL) {
			delete[] pHdr;
			pHdr = NULL;
		}
		pHdr = new uint8_t[len + 1];
		memcpy(pHdr, data, len);
	}

	void SetData(const uint8_t* data, int len) {
		if (len <= 0)
			return;
		nLen = len;
		if (pData != NULL) {
			delete[] pData;
			pData = NULL;
		}
		pData = new uint8_t[len + 1];
		pData[len] = '\0';
		memcpy(pData, data, len);
	}

	uint8_t* pHdr; 
	int nHdrLen;

	int nDataType;
	uint8_t* pData;
	int nLen;

	bool bSendOnce;
	int64_t recvTime;

	struct sockaddr peerAddr;
};
TemplateX2RtcRef(X2NetData);

class X2NetConnection
{
public:
	class Listener
	{
	public:
		virtual ~Listener() = default;

	public:
		virtual void OnX2NetConnectionPacketReceived(
			X2NetConnection* connection, x2rtc::scoped_refptr<X2NetDataRef> x2NetData) = 0;


		virtual void OnX2NetConnectionClosed(X2NetConnection* connection) = 0;

		virtual void OnX2NetRemoteIpPortChanged(X2NetConnection* connection, struct sockaddr* remoteAddr) {};

		virtual void OnX2NetGotNetStats(X2NetConnection* connection, bool isAudio, int nRtt, int nLossRate, int nBufSize) {};
	};

public:
	X2NetConnection() :cb_listener_(NULL), b_force_closed_(false), b_writable_(true), write_cache_data_(NULL), n_write_cache_size_(0), n_write_cache_len_(0){};
	virtual ~X2NetConnection(void) {
		if (write_cache_data_ != NULL) {
			delete[] write_cache_data_;
			write_cache_data_ = NULL;
		}
	};

	virtual bool CanDelete() = 0;

	void SetListener(Listener* listener) {
		cb_listener_ = listener;
	};

	void RecvData(const uint8_t* data, size_t len) {
		if (cb_listener_ != NULL) {
			x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
			x2NetData->SetData(data, len);

			cb_listener_->OnX2NetConnectionPacketReceived(this, x2NetData);
		}

		//printf("RecvData: %.*d\r\n", len, data);
	}

	void RecvDataWithAddr(const uint8_t* data, size_t len, const struct sockaddr* addr) {
		if (cb_listener_ != NULL) {
			x2rtc::scoped_refptr<X2NetDataRef> x2NetData = new x2rtc::RefCountedObject<X2NetDataRef>();
			x2NetData->SetData(data, len);
			x2NetData->peerAddr = *addr;

			cb_listener_->OnX2NetConnectionPacketReceived(this, x2NetData);
		}

		//printf("RecvData: %.*d\r\n", len, data);
	}

	void CacheData(uint8_t* pData, int nLen) {
		if ((n_write_cache_size_ - n_write_cache_len_) < nLen) {
			n_write_cache_size_ = n_write_cache_len_ + nLen;
			uint8_t* pTemp = new uint8_t[n_write_cache_size_];
			if (n_write_cache_len_ > 0) {
				memcpy(pTemp, write_cache_data_, n_write_cache_len_);
			}
			if (write_cache_data_ != NULL) {
				delete[] write_cache_data_;
				write_cache_data_ = NULL;
			}
			write_cache_data_ = pTemp;
			
		}

		if (write_cache_data_ != NULL) {
			memcpy(write_cache_data_ + n_write_cache_len_, pData, nLen);
			n_write_cache_len_ += nLen;
		}
	}
	void ResetCacheData(int nSended) {
		if (n_write_cache_len_ >= nSended) {
			n_write_cache_len_ -= nSended;
			if (n_write_cache_len_ > 0) {
				memmove(write_cache_data_, write_cache_data_ + nSended, n_write_cache_len_);
			}
		}
		else {
			//X2RtcCheck(false);
		}
	}

	virtual void Send(x2rtc::scoped_refptr<X2NetDataRef> x2NetData) {
		JMutexAutoLock l(cs_list_send_data_);
		list_send_data_.push_back(x2NetData);
	};

	//virtual void Send(const uint8_t* data, size_t len) = 0;
	//virtual void Send(const uint8_t* header, size_t hdrLen, const uint8_t* data, size_t len) = 0;

	virtual void ForceClose() {
		b_force_closed_ = true;
	};

	virtual bool IsForceClosed() {
		return b_force_closed_;
	}

	virtual x2rtc::scoped_refptr<X2NetDataRef> GetX2NetData() {
		x2rtc::scoped_refptr<X2NetDataRef> x2NetData = nullptr;
		{
			JMutexAutoLock l(cs_list_send_data_);
			if (list_send_data_.size() > 0) {
				x2NetData = list_send_data_.front();
				list_send_data_.pop_front();
			}
		}
		return x2NetData;
	}

	const std::string& GetRemoteIpPort() {
		return str_remote_ip_port_;
	}

	const std::string& GetStreamId() {
		return str_stream_id_;
	}

	void NotifyWritable(bool bEnable) {
		b_writable_ = bEnable;
	};

	bool IsWritable() {
		return b_writable_;
	}

protected:
	Listener* cb_listener_;

	std::string str_remote_ip_port_;

	std::string str_stream_id_;

private:
	bool b_force_closed_;		// Force to close the connection by system
	bool b_writable_;

	JMutex cs_list_send_data_;
	std::list< x2rtc::scoped_refptr<X2NetDataRef>> list_send_data_;

protected:
	uint8_t* write_cache_data_;
	int n_write_cache_size_;
	int n_write_cache_len_;
};

}	// namespace x2rtc
#endif	// __X2_NET_CONNECTION_H__
