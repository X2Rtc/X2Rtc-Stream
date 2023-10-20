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
#ifndef __X2_SVR_TMPLATE_H__
#define __X2_SVR_TMPLATE_H__
#include <iostream>
#include "X2Config.h"
#include "X2RtcLog.h"
#ifndef WIN32
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

void handle_pipe(int sig)
{//* For UvLib
	//do nothing
	/*��һ���Ѿ��յ�FIN����socket����read����, ������ջ����ѿ�, �򷵻�0, ����ǳ�˵�ı�ʾ���ӹر�. ����һ�ζ������write����ʱ, ������ͻ���û����, �᷵����ȷд��(����). �����͵ı��Ļᵼ�¶Զ˷���RST����, ��Ϊ�Զ˵�socket�Ѿ�������close, ��ȫ�ر�, �Ȳ�����, Ҳ����������. ����, �ڶ��ε���write����(�������յ�RST֮��), ������SIGPIPE�ź�, ���½����˳�.
	*/
	X2RtcPrintf(CRIT, "Handle_pipe sig: %d", sig);
}

#define X2SVR_INIT \
	if (argc == 2) {	\
		if (strcmp(argv[1], "-v") == 0) { \
			X2RtcPrintf(INF, "Version: %s", kVersion); \
			return 0;	\
		}	\
	}	\
	printInfo();	\
	X2ConfigSet	conf;	\
	if (argc > 1)	\
		conf.LoadFromFile(argv[1]);	\
	else	\
	{	\
		std::cout << "Error: Please usage: $0 {conf_path} " << std::endl;	\
		std::cout << "Please enter any key to exit ..." << std::endl;	\
		char c = getchar();	\
		exit(0);	\
	}	\
	{/* Log file	*/		\
		int log_level = conf.GetIntVal("log", "level", 5);	\
		std::string log_path = conf.GetValue("log", "file");	\
		int max_file_size = conf.GetIntVal("log", "max_file_size", 0);	\
		if(log_level == 6){ \
		} \
		else if (log_level < 0 || log_level > 5) {	\
			std::cout << "Error: Log level=" << log_level << " extend range(0 - 5)!" << std::endl;	\
			std::cout << "Please enter any key to exit ..." << std::endl;	\
			getchar();	\
			exit(0);	\
		}	\
		if (log_path.length() > 0) {	\
			OpenX2RtcLog(log_path.c_str(), log_level, max_file_size); \
		}	\
	}	

#ifndef WIN32
#define X2SIGNAL_PIPE \
	struct sigaction sa;	\
	sa.sa_handler = handle_pipe;	\
	sigemptyset(&sa.sa_mask);	\
	sa.sa_flags = 0;	\
	sigaction(SIGPIPE, &sa, NULL);	
#else
#define X2SIGNAL_PIPE
#endif

#endif	// __X2_SVR_TMPLATE_H__