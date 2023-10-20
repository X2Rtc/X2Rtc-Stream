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
# Build all
if [ "$1" = "help" ]; then
	printf "<X2RTC build support list> \r\n"
	printf " abseil-cpp faac faad HttpParser jthread flv mpeg srtp uv MediaSoup webrtc_ms_base opus usrsctp XWsSocket ZLXKit webrtc_ms_media X2ModFlv X2ModRtc X2ModRtmp X2ModRtp X2ModSrt X2ModSrt X2NetUv X2UVBase X2RtcAudCodec X2RtcStream X2WsPlayer PRJ_NAME  \r\n"
fi


 if [ "$1" = "all" ] || [ "$1" = "abseil-cpp" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd third_party/abseil-cpp 
 	printf "Building abseil-cpp....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "faac" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd third_party/faac-1.28 
 	printf "Building faac....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "faad" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd third_party/faad2-2.7 
 	printf "Building faad....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "HttpParser" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd third_party/HttpParser 
 	printf "Building HttpParser....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "jthread" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd third_party/jthread 
 	printf "Building jthread....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "flv" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd third_party/libflv 
 	printf "Building flv....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "mpeg" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd third_party/libmpeg 
 	printf "Building mpeg....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "srtp" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd third_party/libsrtp 
 	printf "Building srtp....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "uv" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd third_party/libuv 
 	printf "Building uv....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "MediaSoup" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd third_party/MediaSoup 
 	printf "Building MediaSoup....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "webrtc_ms_base" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd third_party/MediaSoup/deps/libwebrtc 
 	printf "Building webrtc_ms_base....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "opus" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd third_party/opus 
 	printf "Building opus....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "usrsctp" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd third_party/usrsctp 
 	printf "Building usrsctp....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "XWsSocket" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd third_party/XWsSocket 
 	printf "Building XWsSocket....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "ZLXKit" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd third_party/ZLXKit 
 	printf "Building ZLXKit....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "webrtc_ms_media" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd X2Rtc-Stream/webrtc_media 
 	printf "Building webrtc_ms_media....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "X2ModFlv" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd X2Rtc-Stream/X2Mod-Flv 
 	printf "Building X2ModFlv....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "X2ModRtc" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd X2Rtc-Stream/X2Mod-Rtc 
 	printf "Building X2ModRtc....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "X2ModRtmp" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd X2Rtc-Stream/X2Mod-Rtmp 
 	printf "Building X2ModRtmp....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "X2ModRtp" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd X2Rtc-Stream/X2Mod-Rtp 
 	printf "Building X2ModRtp....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "X2ModSrt" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd X2Rtc-Stream/X2Mod-Srt 
 	printf "Building X2ModSrt....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "X2ModSrt" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd X2Rtc-Stream/X2Mod-Talk 
 	printf "Building X2ModSrt....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "X2NetUv" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd X2Rtc-Stream/X2Net-Uv 
 	printf "Building X2NetUv....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "X2UVBase" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd X2Rtc-Stream/X2UV-Base 
 	printf "Building X2UVBase....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "X2RtcAudCodec" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd X2Rtc-AudCodec 
 	printf "Building X2RtcAudCodec....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 


 if [ "$1" = "all" ] || [ "$1" = "X2RtcStream" ] || [ "$1" = "clean" ]; then 
 	# 
 	cd X2Rtc-Stream 
 	printf "Building X2RtcStream....\r\n" 
 	make $2 
 	cd - 
 	printf "\r\n" 
 fi 



