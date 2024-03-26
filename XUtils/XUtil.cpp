/*
 *  Copyright (c) 2020 The anyRTC project authors. All Rights Reserved.
 *
 *  Website: https://www.x2rtc.io for more details.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "XUtil.h"
#include <limits.h>
#include <memory>
#include <stdexcept>
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <mmsystem.h>
#include <sys/timeb.h>
#include <shlwapi.h>
#include <ws2tcpip.h>
#include <wchar.h>
#ifdef WIN_ARM
#pragma comment(lib,"mmtimer.lib")
#else
#pragma comment(lib,"winmm.lib")
#endif
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Shlwapi.lib")	// __imp__PathIsDirectoryA@4
#pragma comment(lib,"netapi32.lib") // _Netbios@4
#ifdef DEPS_UV
#pragma comment(lib, "IPHLPAPI.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Userenv.lib")
#endif
#else
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#endif

static const int64_t kNumMillisecsPerSec = INT64_C(1000);
static const int64_t kNumMicrosecsPerSec = INT64_C(1000000);
static const int64_t kNumNanosecsPerSec = INT64_C(1000000000);

#ifdef _WIN32
static const uint64_t kFileTimeToUnixTimeEpochOffset = 116444736000000000ULL;

// Emulate POSIX gettimeofday().
// Based on breakpad/src/third_party/glog/src/utilities.cc
static int gettimeofday(struct timeval* tv, void* tz) {
	// FILETIME is measured in tens of microseconds since 1601-01-01 UTC.
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);

	LARGE_INTEGER li;
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;

	// Convert to seconds and microseconds since Unix time Epoch.
	uint64_t micros = (li.QuadPart - kFileTimeToUnixTimeEpochOffset) / 10;
	tv->tv_sec = static_cast<long>(micros / kNumMicrosecsPerSec);  // NOLINT
	tv->tv_usec = static_cast<long>(micros % kNumMicrosecsPerSec); // NOLINT

	return 0;
}
#endif


int64_t TimeUTCMicros() {
#ifdef _WIN32
	struct _timeb time;
	_ftime(&time);
	// Convert from second (1.0) and milliseconds (1e-3).
	return (static_cast<int64_t>(time.time) * kNumMicrosecsPerSec +
		static_cast<int64_t>(time.millitm) * 1000);
#else
	struct timeval time;
	gettimeofday(&time, nullptr);
	// Convert from second (1.0) and microsecond (1e-6).
	return (static_cast<int64_t>(time.tv_sec) * kNumMicrosecsPerSec +
		time.tv_usec);
#endif
	return 0;
}

int64_t TimeUTCMillis() {
	return TimeUTCMicros() / 1000;
}

uint32_t XGetTimestamp(void)
{
#ifdef _WIN32
	return ::timeGetTime();//毫秒
#else
	struct timeval now;
	gettimeofday(&now, NULL);
	return now.tv_sec * 1000 + now.tv_usec / 1000;
#endif
}

uint32_t XGetTimeSecond()
{
	struct timeval timeval;
	if (gettimeofday(&timeval, NULL) < 0) {
		// Incredibly unlikely code path.
		timeval.tv_sec = timeval.tv_usec = 0;
	}
	return static_cast<uint32_t>(timeval.tv_sec);
}

int64_t XGetUtcTimestamp(void)
{
	return TimeUTCMillis();
}


uint16_t XGetLastSequence(uint16_t usSequence1,uint16_t usSequence2)
{
	uint16_t usLastSequence=usSequence2;
	if ((usSequence1>usSequence2 && (uint16_t)(usSequence1-usSequence2)<32768) ||
		(usSequence1<usSequence2 && (uint16_t)(usSequence2-usSequence1)>32768))
	{
		usLastSequence=usSequence1;
	}

	return usLastSequence;
}

uint32_t XGenerateSSRC(void)
{
#ifdef _WIN32
	LARGE_INTEGER frequence, privious;
	if(!QueryPerformanceFrequency( &frequence))//取高精度运行计数器的频率
	{
		return timeGetTime();
	}

	if (!QueryPerformanceCounter( &privious ))
	{
		return timeGetTime();
	}

	DWORD dwRet = (DWORD)(1000000 * privious.QuadPart / frequence.QuadPart ); //换算到微秒数

	return dwRet;//微秒
#else
	struct timeval now;
	gettimeofday(&now,NULL);
	return now.tv_sec*1000+now.tv_usec; 
#endif
}

uint32_t XGetRandomNum()
{//毫秒
#ifdef _WIN32
	struct timeb time_seed;
	ftime(&time_seed);
	srand(time_seed.time * 1000 + time_seed.millitm);
#else
	srand((unsigned)time(NULL));
#endif

	return rand();
}

void XSleep(uint32_t ulMS)
{
#ifdef _WIN32
	Sleep(ulMS);
#else
	usleep(ulMS*1000);
#endif
}

int XSplitChar(const std::string& source, char delimiter,
	std::vector<std::string>* fields) {
	fields->clear();
	size_t last = 0;
	for (size_t i = 0; i < source.length(); ++i) {
		if (source[i] == delimiter) {
			fields->push_back(source.substr(last, i - last));
			last = i + 1;
		}
	}
	fields->push_back(source.substr(last, source.length() - last));
	return fields->size();
}

void XGetRandomStr(std::string&sRandStr, int len) {
	//毫秒
#ifdef _WIN32
	struct timeb time_seed;
	ftime(&time_seed);
	srand(time_seed.time * 1000 + time_seed.millitm);
#else
	srand((unsigned)time(NULL));
#endif

	for (int i = 0; i < len; ++i) {
		switch (rand() % 3) {
		case 1:
			sRandStr += ('A' + rand() % 26);
			break;
		case 2:
			sRandStr += ('a' + rand() % 26);
			break;
		default:
			sRandStr += ('0' + rand() % 10);
			break;
		}
	}

	{//* 防止产生相同的ID
		static uint32_t gInx = 0;
		((char*)sRandStr.c_str())[len - 1] = ('a' + gInx++ % 26);
		if (gInx % 26 == 0) {
			XSleep(1);
		}
	}
}
bool IsIp(std::string str) {
	for (size_t i = 0; i < str.length(); i++) {
		int c = str.c_str()[i];
		if (c != '.' && (c < '0' || c > '9')) {
			return false;
		}
	}
	return true;
}
int GetValByTimeUsed(int nBytes, float fTimeUsed)
{
	float fVal = (float)(nBytes) / fTimeUsed;
	return (int)fVal;
}
void CreateRandomString(std::string& sRandStr, uint32_t len)
{
	return XGetRandomStr(sRandStr, len);
}

int XParseHttpParam(const std::string& strParam, std::map<std::string, std::string>* mapParam)
{
	std::vector<std::string> arrUrl;
	XSplitChar(strParam.c_str(), '&', &arrUrl);
	if (arrUrl.size() > 0) {
		std::vector<std::string>::iterator iter = arrUrl.begin();
		while (iter != arrUrl.end()) {
			std::vector<std::string> arrParam;
			XSplitChar((*iter).c_str(), '=', &arrParam);
			if (arrParam.size() >= 2) {
				(*mapParam)[arrParam[0]] = arrParam[1];
			}
			iter++;
		}
	}
	return 0;
}

int XParseCustomParam(const std::string& strParam, std::map<std::string, std::string>* mapParam, char* arrSp, int nSp)
{
	if (nSp <= 0) {
		return -1;
	}
	std::vector<std::string> arrSp1;
	XSplitChar(strParam.c_str(), arrSp[0], &arrSp1);
	if (nSp > 1) {
		std::vector<std::string>::iterator iter = arrSp1.begin();
		while (iter != arrSp1.end()) {
			std::vector<std::string> arrParam;
			XSplitChar((*iter).c_str(), arrSp[1], &arrParam);
			if (arrParam.size() >= 2) {
				(*mapParam)[arrParam[0]] = arrParam[1];
			}
			iter++;
		}
	}
	else {
		if (arrSp1.size() >= 2) {
			(*mapParam)[arrSp1[0]] = arrSp1[1];
		}
	}
	
	return 0;
}

bool XIsFile(std::string path)
{
	FILE* fp = fopen(path.c_str(), "rb");
	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
		return true;
	}

	return false;
}

#ifndef WIN32
//////////////////////////////////////////////////////////////////////////
//! Log4zFileHandler
//////////////////////////////////////////////////////////////////////////
class Log4zFileHandler
{
public:
	Log4zFileHandler() { _file = NULL; }
	~Log4zFileHandler() { close(); }
	inline bool isOpen() { return _file != NULL; }
	inline long open(const char* path, const char* mod)
	{
		if (_file != NULL) { fclose(_file); _file = NULL; }
		_file = fopen(path, mod);
		if (_file)
		{
			long tel = 0;
			long cur = ftell(_file);
			fseek(_file, 0L, SEEK_END);
			tel = ftell(_file);
			fseek(_file, cur, SEEK_SET);
			return tel;
		}
		return -1;
	}
	inline void clean(int index, int len)
	{
#if !defined(__APPLE__) && !defined(WIN32) && !defined(ANDROID) 
		if (_file != NULL)
		{
			int fd = fileno(_file);
			fsync(fd);
			posix_fadvise(fd, index, len, POSIX_FADV_DONTNEED);
			fsync(fd);
		}
#endif
	}
	inline void close()
	{
		if (_file != NULL) { clean(0, 0); fclose(_file); _file = NULL; }
	}
	inline void write(const char* data, size_t len)
	{
		if (_file && len > 0)
		{
			if (fwrite(data, 1, len, _file) != len)
			{
				close();
			}
		}
	}
	inline void flush() { if (_file) fflush(_file); }

	inline std::string readLine()
	{
		char buf[500] = { 0 };
		if (_file && fgets(buf, 500, _file) != NULL)
		{
			return std::string(buf);
		}
		return std::string();
	}
	inline const std::string readContent();
	inline bool removeFile(const std::string& path) { return ::remove(path.c_str()) == 0; }
public:
	FILE* _file;
};
#endif

bool XIsDirectory(std::string path)
{
#ifdef WIN32
	return PathIsDirectoryA(path.c_str()) ? true : false;
#else
	DIR* pdir = opendir(path.c_str());
	if (pdir == NULL)
	{
		return false;
	}
	else
	{
		closedir(pdir);
		pdir = NULL;
		return true;
	}
#endif
}


static void fixPath(std::string& path)
{
	if (path.empty()) { return; }
	for (std::string::iterator iter = path.begin(); iter != path.end(); ++iter)
	{
		if (*iter == '\\') { *iter = '/'; }
	}
	if (path.at(path.length() - 1) != '/') { path.append("/"); }
}

bool XCreateRecursionDir(std::string path)
{
	if (path.length() == 0) return true;
	std::string sub;
	fixPath(path);

	std::string::size_type pos = path.find('/');
	while (pos != std::string::npos)
	{
		std::string cur = path.substr(0, pos - 0);
		if (cur.length() > 0 && !XIsDirectory(cur))
		{
			bool ret = false;
#ifdef WIN32
			ret = CreateDirectoryA(cur.c_str(), NULL) ? true : false;
#else
			ret = (mkdir(cur.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0);
#endif
			if (!ret)
			{
				return false;
			}
		}
		pos = path.find('/', pos + 1);
	}

	return true;
}

std::string XGetProcessID()
{
	std::string pid = "0";
	char buf[260] = { 0 };
#ifdef WIN32
	DWORD winPID = GetCurrentProcessId();
	sprintf(buf, "%06u", winPID);
	pid = buf;
#else
	sprintf(buf, "%06d", getpid());
	pid = buf;
#endif
	return pid;
}


std::string XGetProcessName()
{
	std::string name = "process";
	char buf[260] = { 0 };
#ifdef WIN32
	if (GetModuleFileNameA(NULL, buf, 259) > 0)
	{
		name = buf;
	}
	std::string::size_type pos = name.rfind("\\");
	if (pos != std::string::npos)
	{
		name = name.substr(pos + 1, std::string::npos);
	}
	pos = name.rfind(".");
	if (pos != std::string::npos)
	{
		name = name.substr(0, pos - 0);
	}

#elif defined(LOG4Z_HAVE_LIBPROC)
	proc_name(getpid(), buf, 260);
	name = buf;
	return name;;
#else
	sprintf(buf, "/proc/%d/cmdline", (int)getpid());
	Log4zFileHandler i;
	i.open(buf, "rb");
	if (!i.isOpen())
	{
		return name;
	}
	name = i.readLine();
	i.close();

	std::string::size_type pos = name.rfind("/");
	if (pos != std::string::npos)
	{
		name = name.substr(pos + 1, std::string::npos);
	}
#endif
	return name;
}
#ifdef WIN32
BOOL getBoisIDByCmd(char* lpszBaseBoard);
#else 
std::string exec(const char* cmd);
#endif
std::string XGetOsUUId()
{
	std::string strOsUUid;
#ifndef WIN32
try {
	strOsUUid = exec("dmidecode -s system-uuid");
}
catch (std::exception& e) {
}
#else
#if 0
	//Generate UUID on Windows
	GUID guid;
	CoCreateGuid(&guid);
	char uuidString[40];
	sprintf(uuidString, "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
	strOsUUid = uuidString;
#else
	char uuidString[1024] = { 0 };
	if (getBoisIDByCmd(uuidString)) {
		strOsUUid = uuidString;
	}
#endif
#endif
	if (strOsUUid.length() == 0) {
		strOsUUid = "X2RtcUniqueId";
	}
	else {
		size_t l = strOsUUid.length();
		if (l && strOsUUid.c_str()[l - 1] == '\n') {
			((char*)strOsUUid.c_str())[l - 1] = '\0';
		}
	}
	return strOsUUid;
}

#ifdef WIN32
std::string ToUtf8(const wchar_t* wide, size_t len) {
	if (len == 0)
		return std::string();
	int len8 = ::WideCharToMultiByte(CP_UTF8, 0, wide, static_cast<int>(len),
		nullptr, 0, nullptr, nullptr);
	std::string ns(len8, 0);
	::WideCharToMultiByte(CP_UTF8, 0, wide, static_cast<int>(len), &*ns.begin(),
		len8, nullptr, nullptr);
	return ns;
}
#endif

std::string XGetCurrentDirectory()
{
#ifndef WIN32
	char buffer[PATH_MAX];
	char* path = getcwd(buffer, PATH_MAX);

	if (!path) {
		//RTC_LOG_ERR(LS_ERROR) << "getcwd() failed";
		return "";  // returns empty pathname
	}
	return path;
#else
	int path_len = 0;
	std::unique_ptr<wchar_t[]> path;
	do {
		int needed = ::GetCurrentDirectoryW(path_len, path.get());
		if (needed == 0) {
			// Error.
			//RTC_LOG(LS_ERROR) << "::GetCurrentDirectory() failed";
			return "";  // returns empty pathname
		}
		if (needed <= path_len) {
			// It wrote successfully.
			break;
		}
		// Else need to re-alloc for "needed".
		path.reset(new wchar_t[needed]);
		path_len = needed;
	} while (true);
	return ToUtf8(path.get(), wcslen(path.get()));
#endif
}

bool XIsIPv4(const std::string& rStr) {
	struct sockaddr_in lSa = { 0 };
	return inet_pton(AF_INET, rStr.c_str(), &(lSa.sin_addr)) != 0;
}

bool XIsIPv6(const std::string& rStr) {
	struct sockaddr_in6 sa = { 0 };
	return inet_pton(AF_INET6, rStr.c_str(), &(sa.sin6_addr)) != 0;
}

int16_t X2ReadShort(char** pptr)
{
	char* ptr = *pptr;
	uint16_t len = 256 * ((unsigned char)(*ptr)) + (unsigned char)(*(ptr + 1));
	*pptr += 2;
	return len;
}
void X2WriteShort(char** pptr, uint16_t anInt)
{
	**pptr = (char)(anInt / 256); (*pptr)++;
	**pptr = (char)(anInt % 256); (*pptr)++;
}


///////////////////////////////////////////////////////////////////////////
#include <assert.h>
enum AVRounding {
	AV_ROUND_ZERO = 0, ///< Round toward zero.
	AV_ROUND_INF = 1, ///< Round away from zero.
	AV_ROUND_DOWN = 2, ///< Round toward -infinity.
	AV_ROUND_UP = 3, ///< Round toward +infinity.
	AV_ROUND_NEAR_INF = 5, ///< Round to nearest and halfway cases away from zero.
	AV_ROUND_PASS_MINMAX = 8192, ///< Flag to pass INT64_MIN/MAX through instead of rescaling, this avoids special cases for AV_NOPTS_VALUE
};

#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMAX3(a,b,c) FFMAX(FFMAX(a,b),c)
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define FFMIN3(a,b,c) FFMIN(FFMIN(a,b),c)

int64_t av_rescale_rnd(int64_t a, int64_t b, int64_t c, int rnd)
{
	int64_t r = 0;
	assert(c > 0);
	assert(b >= 0);
	assert((unsigned)(rnd & ~AV_ROUND_PASS_MINMAX) <= 5 && (rnd & ~AV_ROUND_PASS_MINMAX) != 4);

	if (c <= 0 || b < 0 || !((unsigned)(rnd & ~AV_ROUND_PASS_MINMAX) <= 5 && (rnd & ~AV_ROUND_PASS_MINMAX) != 4))
		return INT64_MIN;

	if (rnd & AV_ROUND_PASS_MINMAX) {
		if (a == INT64_MIN || a == INT64_MAX)
			return a;
		rnd -= AV_ROUND_PASS_MINMAX;
	}

	if (a < 0)
	{
		int64_t ret = (uint64_t)av_rescale_rnd(-FFMAX(a, -INT64_MAX), b, c, rnd ^ ((rnd >> 1) & 1));
		return -ret;	//-(uint64_t)av_rescale_rnd(-FFMAX(a, -INT64_MAX), b, c, rnd ^ ((rnd >> 1) & 1));
	}

	if (rnd == AV_ROUND_NEAR_INF)
		r = c / 2;
	else if (rnd & 1)
		r = c - 1;

	if (b <= INT_MAX && c <= INT_MAX) {
		if (a <= INT_MAX)
			return (a * b + r) / c;
		else {
			int64_t ad = a / c;
			int64_t a2 = (a % c * b + r) / c;
			if (ad >= INT32_MAX && b && ad > (INT64_MAX - a2) / b)
				return INT64_MIN;
			return ad * b + a2;
		}
	}
	else {
		uint64_t a0 = a & 0xFFFFFFFF;
		uint64_t a1 = a >> 32;
		uint64_t b0 = b & 0xFFFFFFFF;
		uint64_t b1 = b >> 32;
		uint64_t t1 = a0 * b1 + a1 * b0;
		uint64_t t1a = t1 << 32;
		int i;

		a0 = a0 * b0 + t1a;
		a1 = a1 * b1 + (t1 >> 32) + (a0 < t1a);
		a0 += r;
		a1 += a0 < r;

		for (i = 63; i >= 0; i--) {
			a1 += a1 + ((a0 >> i) & 1);
			t1 += t1;
			if (c <= a1) {
				a1 -= c;
				t1++;
			}
		}
		if (t1 > INT64_MAX)
			return INT64_MIN;
		return t1;
	}
}

int64_t X2AvRescale(int64_t a, int64_t b, int64_t c)
{
	return av_rescale_rnd(a, b, c, AV_ROUND_NEAR_INF);
}

int SplitChar(const std::string& source, char delimiter,
	std::vector<std::string>* fields) {
	fields->clear();
	size_t last = 0;
	for (size_t i = 0; i < source.length(); ++i) {
		if (source[i] == delimiter) {
			fields->push_back(source.substr(last, i - last));
			last = i + 1;
		}
	}
	fields->push_back(source.substr(last, source.length() - last));
	return fields->size();
}

std::string ReplaceStrOnce(const std::string& strSrc, const std::string& strFind, const std::string& strReplace)
{
	std::string strStr;
	std::string::size_type pos;
	std::string strCopy = strSrc;
	if ((pos = strCopy.find(strFind)) != std::string::npos) {
		strStr = strCopy.replace(pos, strFind.length(), strReplace.c_str());
	}
	else {
		strStr = strCopy;
	}
	return strStr;
}

////////////////////////////////////////////////////////////////////////
//帧类型
enum Frametype_e
{
	FRAME_I = 15,
	FRAME_P = 16,
	FRAME_B = 17
};
//读取字节结构体
typedef struct Tag_bs_t
{
	unsigned char* p_start;                //缓冲区首地址(这个开始是最低地址)
	unsigned char* p;                      //缓冲区当前的读写指针 当前字节的地址，这个会不断的++，每次++，进入一个新的字节
	unsigned char* p_end;                  //缓冲区尾地址     //typedef unsigned char   uint8_t;
	int     i_left;                        // p所指字节当前还有多少 “位” 可读写 count number of available(可用的)位
}bs_t;
/*
 函数名称：
 函数功能：初始化结构体
 参    数：
 返 回 值：无返回值,void类型
 思    路：
 资    料：

 */
void bs_init(bs_t* s, void* p_data, int i_data)
{
	s->p_start = (unsigned char*)p_data;        //用传入的p_data首地址初始化p_start，只记下有效数据的首地址
	s->p = (unsigned char*)p_data;        //字节?首地址，一开始用p_data初始化，每读完一个整字节，就移动到下一字节首地址
	s->p_end = s->p + i_data;                   //尾地址，最后一个字节的首地址?
	s->i_left = 8;                              //还没有开始读写，当前字节剩余未读取的位是8
}
int bs_read(bs_t* s, int i_count)
{
	static uint32_t i_mask[33] = { 0x00,
		0x01,      0x03,      0x07,      0x0f,
		0x1f,      0x3f,      0x7f,      0xff,
		0x1ff,     0x3ff,     0x7ff,     0xfff,
		0x1fff,    0x3fff,    0x7fff,    0xffff,
		0x1ffff,   0x3ffff,   0x7ffff,   0xfffff,
		0x1fffff,  0x3fffff,  0x7fffff,  0xffffff,
		0x1ffffff, 0x3ffffff, 0x7ffffff, 0xfffffff,
		0x1fffffff,0x3fffffff,0x7fffffff,0xffffffff };
	/*
	 数组中的元素用二进制表示如下：

	 假设：初始为0，已写入为+，已读取为-

	 字节:       1       2       3       4
	 00000000 00000000 00000000 00000000      下标

	 0x00:                           00000000      x[0]

	 0x01:                           00000001      x[1]
	 0x03:                           00000011      x[2]
	 0x07:                           00000111      x[3]
	 0x0f:                           00001111      x[4]

	 0x1f:                           00011111      x[5]
	 0x3f:                           00111111      x[6]
	 0x7f:                           01111111      x[7]
	 0xff:                           11111111      x[8]    1字节

	 0x1ff:                      0001 11111111      x[9]
	 0x3ff:                      0011 11111111      x[10]   i_mask[s->i_left]
	 0x7ff:                      0111 11111111      x[11]
	 0xfff:                      1111 11111111      x[12]   1.5字节

	 0x1fff:                  00011111 11111111      x[13]
	 0x3fff:                  00111111 11111111      x[14]
	 0x7fff:                  01111111 11111111      x[15]
	 0xffff:                  11111111 11111111      x[16]   2字节

	 0x1ffff:             0001 11111111 11111111      x[17]
	 0x3ffff:             0011 11111111 11111111      x[18]
	 0x7ffff:             0111 11111111 11111111      x[19]
	 0xfffff:             1111 11111111 11111111      x[20]   2.5字节

	 0x1fffff:         00011111 11111111 11111111      x[21]
	 0x3fffff:         00111111 11111111 11111111      x[22]
	 0x7fffff:         01111111 11111111 11111111      x[23]
	 0xffffff:         11111111 11111111 11111111      x[24]   3字节

	 0x1ffffff:    0001 11111111 11111111 11111111      x[25]
	 0x3ffffff:    0011 11111111 11111111 11111111      x[26]
	 0x7ffffff:    0111 11111111 11111111 11111111      x[27]
	 0xfffffff:    1111 11111111 11111111 11111111      x[28]   3.5字节

	 0x1fffffff:00011111 11111111 11111111 11111111      x[29]
	 0x3fffffff:00111111 11111111 11111111 11111111      x[30]
	 0x7fffffff:01111111 11111111 11111111 11111111      x[31]
	 0xffffffff:11111111 11111111 11111111 11111111      x[32]   4字节

	 */
	int      i_shr;             //
	int i_result = 0;           //用来存放读取到的的结果 typedef unsigned   uint32_t;

	while (i_count > 0)     //要读取的比特数
	{
		if (s->p >= s->p_end) //字节流的当前位置>=流结尾，即代表此比特流s已经读完了。
		{                       //
			break;
		}

		if ((i_shr = s->i_left - i_count) >= 0)    //当前字节剩余的未读位数，比要读取的位数多，或者相等
		{                                           //i_left当前字节剩余的未读位数，本次要读i_count比特，i_shr=i_left-i_count的结果如果>=0，说明要读取的都在当前字节内
			//i_shr>=0，说明要读取的比特都处于当前字节内
			//这个阶段，一次性就读完了，然后返回i_result(退出了函数)
			/* more in the buffer than requested */
			i_result |= (*s->p >> i_shr) & i_mask[i_count];//“|=”:按位或赋值，A |= B 即 A = A|B
			//|=应该在最后执行，把结果放在i_result(按位与优先级高于复合操作符|=)
			//i_mask[i_count]最右侧各位都是1,与括号中的按位与，可以把括号中的结果复制过来
			//!=,左边的i_result在这儿全是0，右侧与它按位或，还是复制结果过来了，好象好几步都多余
			/*读取后，更新结构体里的字段值*/
			s->i_left -= i_count; //即i_left = i_left - i_count，当前字节剩余的未读位数，原来的减去这次读取的
			if (s->i_left == 0) //如果当前字节剩余的未读位数正好是0，说明当前字节读完了，就要开始下一个字节
			{
				s->p++;              //移动指针，所以p好象是以字节为步长移动指针的
				s->i_left = 8;       //新开始的这个字节来说，当前字节剩余的未读位数，就是8比特了
			}
			return(i_result);     //可能的返回值之一为：00000000 00000000 00000000 00000001 (4字节长)
		}
		else    /* i_shr < 0 ,跨字节的情况*/
		{
			//这个阶段，是while的一次循环，可能还会进入下一次循环，第一次和最后一次都可能读取的非整字节，比如第一次读了3比特，中间读取了2字节(即2x8比特)，最后一次读取了1比特，然后退出while循环
			//当前字节剩余的未读位数，比要读取的位数少，比如当前字节有3位未读过，而本次要读7位
			//???对当前字节来说，要读的比特，都在最右边，所以不再移位了(移位的目的是把要读的比特放在当前字节最右)
			/* less(较少的) in the buffer than requested */
			i_result |= (*s->p & i_mask[s->i_left]) << -i_shr;    //"-i_shr"相当于取了绝对值
			//|= 和 << 都是位操作符，优先级相同，所以从左往右顺序执行
			//举例:int|char ，其中int是4字节，char是1字节，sizeof(int|char)是4字节
			//i_left最大是8，最小是0，取值范围是[0,8]
			i_count -= s->i_left;   //待读取的比特数，等于原i_count减去i_left，i_left是当前字节未读过的比特数，而此else阶段，i_left代表的当前字节未读的比特全被读过了，所以减它
			s->p++;  //定位到下一个新的字节
			s->i_left = 8;   //对一个新字节来说，未读过的位数当然是8，即本字节所有位都没读取过
		}
	}

	return(i_result);//可能的返回值之一为：00000000 00000000 00000000 00000001 (4字节长)
}
int bs_read1(bs_t* s)
{

	if (s->p < s->p_end)
	{
		unsigned int i_result;

		s->i_left--;                           //当前字节未读取的位数少了1位
		i_result = (*s->p >> s->i_left) & 0x01;//把要读的比特移到当前字节最右，然后与0x01:00000001进行逻辑与操作，因为要读的只是一个比特，这个比特不是0就是1，与0000 0001按位与就可以得知此情况
		if (s->i_left == 0)                   //如果当前字节剩余未读位数是0，即是说当前字节全读过了
		{
			s->p++;                             //指针s->p 移到下一字节
			s->i_left = 8;                     //新字节中，未读位数当然是8位
		}
		return i_result;                       //unsigned int
	}

	return 0;                                  //返回0应该是没有读到东西
}
int bs_read_ue(bs_t* s)
{
	int i = 0;

	while (bs_read1(s) == 0 && s->p < s->p_end && i < 32)    //条件为：读到的当前比特=0，指针未越界，最多只能读32比特
	{
		i++;
	}
	return((1 << i) - 1 + bs_read(s, i));
}
bool X2IsH264BFrame(const char* pData, int nLen)
{
	if (nLen <= 5) {
		return false;
	}
	int nPrefixSize = 4;
	if (pData[0] == 0x0 && pData[1] == 0x0 && pData[2] == 0x1) {
		nPrefixSize = 3;
	}
	bs_t s;
	bs_init(&s, (char*)pData + nPrefixSize + 1, nLen - nPrefixSize - 1);
	{
		/* i_first_mb */
		bs_read_ue(&s);
		/* picture type */
		int frame_type = bs_read_ue(&s);
		Frametype_e ft = FRAME_P;
		switch (frame_type)
		{
		case 0: case 5: /* P */
			ft = FRAME_P;
			break;
		case 1: case 6: /* B */
			ft = FRAME_B;
			break;
		case 3: case 8: /* SP */
			ft = FRAME_P;
			break;
		case 2: case 7: /* I */
			ft = FRAME_I;
			break;
		case 4: case 9: /* SI */
			ft = FRAME_I;
			break;
		}

		if (ft == FRAME_B) {
			return true;
		}
	}
	return false;
}

#ifdef WIN32
BOOL getBoisIDByCmd(char* lpszBaseBoard) {
	const long MAX_COMMAND_SIZE = 10000; // 命令行输出缓冲大小	

#ifndef UNICODE
	CHAR szFetCmd[] = "wmic csproduct get UUID"; // 获取BOIS命令行	
#else
	WCHAR szFetCmd[] = L"wmic csproduct get UUID"; // 获取BOIS命令行	
#endif
	const std::string strEnSearch = "UUID"; // 主板序列号的前导信息

	BOOL   bret = FALSE;
	HANDLE hReadPipe = NULL; //读取管道
	HANDLE hWritePipe = NULL; //写入管道	
	PROCESS_INFORMATION pi; //进程信息	
	memset(&pi, 0, sizeof(pi));
	STARTUPINFO	si;	//控制命令行窗口信息
	memset(&si, 0, sizeof(si));
	SECURITY_ATTRIBUTES sa; //安全属性
	memset(&sa, 0, sizeof(sa));

	char szBuffer[MAX_COMMAND_SIZE + 1] = { 0 }; // 放置命令行结果的输出缓冲区
	std::string	strBuffer;
	unsigned long count = 0;
	long ipos = 0;

	pi.hProcess = NULL;
	pi.hThread = NULL;
	si.cb = sizeof(STARTUPINFO);
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	//1.创建管道
	bret = CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);
	if (!bret) {
		CloseHandle(hWritePipe);
		CloseHandle(hReadPipe);

		return bret;
	}

	//2.设置命令行窗口的信息为指定的读写管道
	GetStartupInfo(&si);
	si.hStdError = hWritePipe;
	si.hStdOutput = hWritePipe;
	si.wShowWindow = SW_HIDE; //隐藏命令行窗口
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

	//3.创建获取命令行的进程
	bret = CreateProcess(NULL, szFetCmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
	if (!bret) {
		CloseHandle(hWritePipe);
		CloseHandle(hReadPipe);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		return bret;
	}

	//4.读取返回的数据
	WaitForSingleObject(pi.hProcess, 200);
	bret = ReadFile(hReadPipe, szBuffer, MAX_COMMAND_SIZE, &count, 0);
	if (!bret) {
		CloseHandle(hWritePipe);
		CloseHandle(hReadPipe);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		return bret;
	}

	//5.查找主板ID
	bret = FALSE;
	strBuffer = szBuffer;
	ipos = strBuffer.find(strEnSearch);

	if (ipos < 0) { // 没有找到
		CloseHandle(hWritePipe);
		CloseHandle(hReadPipe);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		return bret;
	}
	else {
		strBuffer = strBuffer.substr(ipos + strEnSearch.length());
	}

	memset(szBuffer, 0x00, sizeof(szBuffer));
	strcpy_s(szBuffer, strBuffer.c_str());

	//去掉中间的空格 \r \n
	int j = 0;
	for (int i = 0; i < strlen(szBuffer); i++) {
		if (szBuffer[i] != ' ' && szBuffer[i] != '\n' && szBuffer[i] != '\r') {
			lpszBaseBoard[j] = szBuffer[i];
			j++;
		}
	}

	CloseHandle(hWritePipe);
	CloseHandle(hReadPipe);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return TRUE;
}
#endif

#ifndef WIN32
std::string exec(const char* cmd) {
	char buffer[128];
	std::string result = "";
	FILE* pipe = popen(cmd, "r");
	if (!pipe) throw std::runtime_error("popen() failed!");
	try {
		while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
			result += buffer;
		}
	}
	catch (...) {
		pclose(pipe);
		throw;
	}
	pclose(pipe);
	return result;
}
#endif