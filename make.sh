#!/bin/sh
## X2Rtc build script
## https://www.x2rtc.com
CLEAN_CMD=0
BasePath=$(pwd)
if [ "$1" = "clean" ] || [ "$2" = "clean" ]; then
CLEAN_CMD=1
fi
ARM_FLAG=0
if [ "$2" = "arm" ] || [ "$2" = "aarch64-gcc49" ]; then
ARM_FLAG=1
fi

#######################################################
# Check GCC version
gcc_version=`gcc --version | grep GCC | awk '{print $3}' | awk -F. '{print $1}'`
if [[ $gcc_version -lt 5 ]]; then
    echo "ERROR: GCC version < 5"
	echo "GCC version must >= 5"
    exit 1;
fi

#######################################################
# Build all
TARGET_OBJ_PATH="./out/Obj_common"
TARGET_LINUX_PATH="./out/Linux_common"

if [ "$1" = "help" ]; then
	printf "<X2RTC build support list> \r\n"
	printf " abseil-cpp faac faad HttpParser jthread flv mpeg srtp uv MediaSoup webrtc_ms_base opus usrsctp XWsSocket ZLXKit webrtc_ms_media X2ModFlv X2ModRtc X2ModRtmp X2ModRtp X2ModSrt X2ModSrt X2NetUv X2UVBase X2RtcAudCodec X2RtcStream X2WsPlayer PRJ_NAME  \r\n"
fi

 if [ "$1" = "all" ] && [ "$2" = "clean" ] ; then 
	rm -rf out
 else
	yum install -y perl-ExtUtils-CBuilder perl-ExtUtils-MakeMaker tcl
	mkdir -p $TARGET_LINUX_PATH
 fi

 if [ "$1" = "all" ] || [ "$1" = "openssl" ]; then 
	#
	cd x-build
	if [ "$2" = "clean" ];then
		rm -rf openssl
		cd -
	else
		if [ -d "openssl" ];then
			echo "OpenSSL install Make"
			cd ../
		else
			tar -zxvf openssl.tar.gz
			cd openssl
			./Configure --prefix=$pwd/build
			make $2 && make install
			cp -r $pwd/build/lib64/*.so* ../../$TARGET_LINUX_PATH
			cd ../..
		fi
	fi
 fi
 
  if [ "$1" = "all" ] || [ "$1" = "srt" ]; then 
 	# 
	cd x-build
	if [ "$2" = "clean" ];then
		rm -rf srt
		cd -
	else
		if [ -d "srt" ];then
			echo "Srt install Make"
			cd ../
		else
			tar -zxvf srt.tar.gz
			cd srt
			./configure --prefix=$pwd/build
			make $2 && make install
			cp -r $pwd/build/lib64/*.so* ../../$TARGET_LINUX_PATH
			cd ../../
		fi
	fi
 fi 

 if [ "$1" = "all" ] || [ "$1" = "abseil-cpp" ] ; then 
 	#
 	cd third_party/abseil-cpp 
 	printf "Building abseil-cpp....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 
 
 if [ "$1" = "all" ] || [ "$1" = "faac" ] ; then 
 	# 
 	cd third_party/faac-1.28 
 	printf "Building faac....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "faad" ] ; then 
 	# 
 	cd third_party/faad2-2.7 
 	printf "Building faad....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "HttpParser" ] ; then 
 	# 
 	cd third_party/HttpParser 
 	printf "Building HttpParser....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "jthread" ] ; then 
 	# 
 	cd third_party/jthread 
 	printf "Building jthread....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "flv" ] ; then 
 	# 
 	cd third_party/libflv 
 	printf "Building flv....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "mpeg" ] ; then 
 	# 
 	cd third_party/libmpeg 
 	printf "Building mpeg....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "srtp" ] ; then 
 	# 
 	cd third_party/libsrtp 
 	printf "Building srtp....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "uv" ] ; then 
 	# 
 	cd third_party/libuv 
 	printf "Building uv....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "MediaSoup" ] ; then 
 	# 
 	cd third_party/MediaSoup 
 	printf "Building MediaSoup....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "webrtc_ms_base" ] ; then 
 	# 
 	cd third_party/MediaSoup/deps/libwebrtc 
 	printf "Building webrtc_ms_base....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "opus" ] ; then 
 	# 
 	cd third_party/opus 
 	printf "Building opus....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "usrsctp" ] ; then 
 	# 
 	cd third_party/usrsctp 
 	printf "Building usrsctp....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "XWsSocket" ] ; then 
 	# 
 	cd third_party/XWsSocket 
 	printf "Building XWsSocket....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "ZLXKit" ] ; then 
 	# 
 	cd third_party/ZLXKit 
 	printf "Building ZLXKit....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "webrtc_ms_media" ] ; then 
 	# 
 	cd X2Rtc-Stream/webrtc_media 
 	printf "Building webrtc_ms_media....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "X2ModFlv" ] ; then 
 	# 
 	cd X2Rtc-Stream/X2Mod-Flv 
 	printf "Building X2ModFlv....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "X2ModRtc" ] ; then 
 	# 
 	cd X2Rtc-Stream/X2Mod-Rtc 
 	printf "Building X2ModRtc....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "X2ModRtmp" ] ; then 
 	# 
 	cd X2Rtc-Stream/X2Mod-Rtmp 
 	printf "Building X2ModRtmp....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "X2ModRtp" ] ; then 
 	# 
 	cd X2Rtc-Stream/X2Mod-Rtp 
 	printf "Building X2ModRtp....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "X2ModSrt" ] ; then 
 	# 
 	cd X2Rtc-Stream/X2Mod-Srt 
 	printf "Building X2ModSrt....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "X2NetUv" ] ; then 
 	# 
 	cd X2Rtc-Stream/X2Net-Uv 
 	printf "Building X2NetUv....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "X2UVBase" ] ; then 
 	# 
 	cd X2Rtc-Stream/X2UV-Base 
 	printf "Building X2UVBase....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "X2RtcAudCodec" ] ; then 
 	# 
 	cd X2Rtc-AudCodec 
 	printf "Building X2RtcAudCodec....\r\n" 
	if [ "$2" = "clean" ];then
		make $2 && rm -rf  out
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
	fi
 	cd - 
 	printf "\r\n" 
 fi 

 if [ "$1" = "all" ] || [ "$1" = "X2RtcStream" ] ; then 
 	# 
 	cd X2Rtc-Stream 
 	printf "Building X2RtcStream....\r\n" 
	if [ "$2" = "clean" ];then
		make $2
	else
		mkdir -p $TARGET_OBJ_PATH
		make $2 && make install
		chmod +x  ../$TARGET_LINUX_PATH/X2RtcStream 
	fi
 	cd - 
 	printf "\r\n" 
 fi 