# Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
#  

TARGET = libZLXKit
C++ := g++
CC := gcc
AR := ar
RANLIB := ranlib
INCLUDE_FLAG = -include
INCPATH = -I./ -I./ZLMediaKit -I./ZLToolKit/src -I../libmpeg/include -I../libflv/include
CCFLAGS += -Wall -fPIC -D_GNU_SOURCE -fvisibility=hidden -DNDEBUG -O2 -DWEBRTC_POSIX -DWEBRTC_LINUX -DMS_LITTLE_ENDIAN -DLOGGING=1 -D__STDC_FORMAT_MACROS -DHAVE_MMSG_HDR -DHAVE_RECVMMSG_API -DHAVE_SENDMMSG_API 
CCFLAGS += -DENABLE_HLS -DENABLE_RTPPROXY -DENABLE_MP4 -DENABLE_JPEG

# EACH DIRECTORY WITH HEADERS MUST BE APPENDED IN THIS MANNER TO THE CCFLAGS
C++FLAGS = $(CCFLAGS)

INSTALL_DIR = ../../out/Linux_common

CFILES = \
	./ZLMediaKit/Extension/SPSParser.c

CCFILES= \
	
CPPFILES = \
	./WastXSdp.cpp \
    ./ZLMediaKit/Common/config.cpp \
    ./ZLMediaKit/Common/macros.cpp \
	./ZLMediaKit/Common/MediaSink.cpp \
    ./ZLMediaKit/Common/Parser.cpp \
    ./ZLMediaKit/Common/Stamp.cpp \
	./ZLMediaKit/Common/strCoding.cpp \
    ./ZLMediaKit/Extension/AAC.cpp \
    ./ZLMediaKit/Extension/AACRtmp.cpp \
    ./ZLMediaKit/Extension/AACRtp.cpp \
    ./ZLMediaKit/Extension/CommonRtmp.cpp \
    ./ZLMediaKit/Extension/CommonRtp.cpp \
    ./ZLMediaKit/Extension/Factory.cpp \
    ./ZLMediaKit/Extension/Frame.cpp \
    ./ZLMediaKit/Extension/G711.cpp \
    ./ZLMediaKit/Extension/G711Rtp.cpp \
    ./ZLMediaKit/Extension/H264.cpp \
    ./ZLMediaKit/Extension/H264Rtmp.cpp \
    ./ZLMediaKit/Extension/H264Rtp.cpp \
    ./ZLMediaKit/Extension/H265.cpp \
    ./ZLMediaKit/Extension/H265Rtmp.cpp \
    ./ZLMediaKit/Extension/H265Rtp.cpp \
    ./ZLMediaKit/Extension/JPEG.cpp \
    ./ZLMediaKit/Extension/JPEGRtp.cpp \
    ./ZLMediaKit/Extension/L16.cpp \
    ./ZLMediaKit/Extension/Opus.cpp \
    ./ZLMediaKit/Http/HttpConst.cpp \
    ./ZLMediaKit/Http/HttpRequestSplitter.cpp \
    ./ZLMediaKit/Http/WebSocketSplitter.cpp \
	./ZLMediaKit/Rtcp/Rtcp.cpp \
    ./ZLMediaKit/Rtcp/RtcpContext.cpp \
    ./ZLMediaKit/Rtcp/RtcpFCI.cpp \
    ./ZLMediaKit/Rtmp/amf.cpp \
	./ZLMediaKit/Rtmp/FlvSplitter.cpp \
    ./ZLMediaKit/Rtmp/Rtmp.cpp \
	./ZLMediaKit/Rtmp/RtmpDemuxer.cpp \
    ./ZLMediaKit/Rtmp/RtmpMediaSource.cpp \
    ./ZLMediaKit/Rtmp/RtmpProtocol.cpp \
    ./ZLMediaKit/Rtmp/utils.cpp \
    ./ZLMediaKit/RtpExt.cpp \
    ./ZLMediaKit/Rtp/Decoder.cpp \
    ./ZLMediaKit/Rtp/PSDecoder.cpp \
    ./ZLMediaKit/Rtp/TSDecoder.cpp \
    ./ZLMediaKit/Rtsp/RtpCodec.cpp \
    ./ZLMediaKit/Rtsp/RtpReceiver.cpp \
    ./ZLMediaKit/Rtsp/Rtsp.cpp \
    ./ZLMediaKit/Sdp.cpp \
    ./ZLToolKit/src/Network/Buffer.cpp \
    ./ZLToolKit/src/Network/sockutil.cpp \
    ./ZLToolKit/src/Util/base64.cpp \
    ./ZLToolKit/src/Util/File.cpp \
	./ZLToolKit/src/Util/local_time.cpp \
    ./ZLToolKit/src/Util/logger.cpp \
    ./ZLToolKit/src/Util/mini.cpp \
    ./ZLToolKit/src/Util/NoticeCenter.cpp \
    ./ZLToolKit/src/Util/SHA1.cpp \
    ./ZLToolKit/src/Util/strptime_win.cpp \
    ./ZLToolKit/src/Util/util.cpp \
    ./ZLToolKit/src/Util/uv_errno.cpp
	

    
	


OUT_OBJ = ./out/Obj_common

CFILES_OBJ := $(patsubst %.c,%.o,$(CFILES))

CC_FILES_OBJ := $(patsubst %.cc,%.o,$(CCFILES))

CPP_FILES_OBJ := $(patsubst %.cpp,%.o,$(CPPFILES))
		
all: $(TARGET).a 

$(TARGET).a: $(CFILES:.c=.o) $(CCFILES:.cc=.o) $(CPPFILES:.cpp=.o) 
	$(AR)  -r $(OUT_OBJ)/$(TARGET).a $(CFILES_OBJ) $(CC_FILES_OBJ) $(CPP_FILES_OBJ) 
	$(RANLIB) $(OUT_OBJ)/$(TARGET).a

install: 
	install -m 664 $(OUT_OBJ)/$(TARGET).a $(INSTALL_DIR)
	
clean:
	rm -f $(OUT_OBJ)/$(TARGET).a $(CFILES_OBJ) $(CC_FILES_OBJ) $(CPP_FILES_OBJ) 

.SUFFIXES: .cc .cpp .c .o

.c.o:
	$(CC) -c -o $*.o $(DEFINES) -std=c99 $(CCFLAGS) $*.c $(INCPATH)
	
.cc.o:
	$(C++) -c -o $*.o $(DEFINES) -std=c++11 $(C++FLAGS) -fpermissive $*.cc $(INCPATH)
	
.cpp.o:
	$(C++) -c -o $*.o $(DEFINES) -std=c++11 $(C++FLAGS) -fpermissive $*.cpp $(INCPATH) 
