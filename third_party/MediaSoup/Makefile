# Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
#  

TARGET = libMediaSoup
C++ := g++
CC := gcc
AR := ar
RANLIB := ranlib
INCLUDE_FLAG = -include
INCPATH = -I./ -I./include -I./deps -I./deps/libwebrtc -I./deps/abseil-cpp -I./deps/json/single_include -I./deps/json/single_include/nlohmann
INCPATH += -I./deps/netstring-c -I./deps/libsdptransform/include -I../ -I../libuv/include 
INCPATH += -I../libsrtp/include -I../openssl/include -I../usrsctp/usrsctplib/usrsctplib
CCFLAGS += -Wall -fPIC -D_GNU_SOURCE -fvisibility=hidden -DNDEBUG -O2 -DWEBRTC_POSIX -DWEBRTC_LINUX -DMS_LITTLE_ENDIAN -DLOGGING=1 -D__STDC_FORMAT_MACROS 


# EACH DIRECTORY WITH HEADERS MUST BE APPENDED IN THIS MANNER TO THE CCFLAGS
C++FLAGS = $(CCFLAGS)

INSTALL_DIR = ../../out/Linux_common

CFILES = \
	./deps/netstring-c/netstring.c

CCFILES= \
	./src/sdp/h264_profile_level_id.cc \

CPPFILES = \
    ./deps/libsdptransform/src/grammar.cpp \
    ./deps/libsdptransform/src/parser.cpp \
    ./deps/libsdptransform/src/writer.cpp \
    ./Log.cpp \
    ./media/Depacketizer.cpp \
    ./rtp/RtpH264Parser.cpp \
    ./SinglePort.cpp \
    ./src/ChannelMessageRegistrator.cpp \
    ./src/Channel/ChannelNotifier.cpp \
    ./src/Channel/ChannelRequest.cpp \
    ./src/Channel/ChannelSocket.cpp \
    ./src/DepLibSRTP.cpp \
    ./src/DepLibUV.cpp \
    ./src/DepLibWebRTC.cpp \
    ./src/DepOpenSSL.cpp \
    ./src/DepUsrSCTP.cpp \
    ./src/handles/SignalsHandler.cpp \
    ./src/handles/TcpConnectionHandler.cpp \
    ./src/handles/TcpServerHandler.cpp \
    ./src/handles/Timer.cpp \
    ./src/handles/UdpSocketHandler.cpp \
    ./src/handles/UnixStreamSocket.cpp \
    ./src/lib.cpp \
    ./src/Logger.cpp \
    ./src/MediaSoupErrors.cpp \
    ./src/PayloadChannel/PayloadChannelNotification.cpp \
    ./src/PayloadChannel/PayloadChannelNotifier.cpp \
    ./src/PayloadChannel/PayloadChannelRequest.cpp \
    ./src/PayloadChannel/PayloadChannelSocket.cpp \
    ./src/RTC/ActiveSpeakerObserver.cpp \
    ./src/RTC/AudioLevelObserver.cpp \
    ./src/RTC/Codecs/H264.cpp \
    ./src/RTC/Codecs/H264_SVC.cpp \
	./src/RTC/Codecs/H265.cpp \
    ./src/RTC/Codecs/Opus.cpp \
    ./src/RTC/Codecs/VP8.cpp \
    ./src/RTC/Codecs/VP9.cpp \
    ./src/RTC/Consumer.cpp \
    ./src/RTC/DataConsumer.cpp \
    ./src/RTC/DataProducer.cpp \
    ./src/RTC/DirectTransport.cpp \
    ./src/RTC/DtlsTransport.cpp \
    ./src/RTC/IceCandidate.cpp \
    ./src/RTC/IceServer.cpp \
    ./src/RTC/KeyFrameRequestManager.cpp \
    ./src/RTC/NackGenerator.cpp \
    ./src/RTC/PipeConsumer.cpp \
    ./src/RTC/PipeTransport.cpp \
    ./src/RTC/PlainTransport.cpp \
    ./src/RTC/PortManager.cpp \
    ./src/RTC/Producer.cpp \
    ./src/RTC/RateCalculator.cpp \
    ./src/RTC/Router.cpp \
	./src/RTC/RtcLogger.cpp \
    ./src/RTC/RTCP/Bye.cpp \
    ./src/RTC/RTCP/CompoundPacket.cpp \
    ./src/RTC/RTCP/Feedback.cpp \
    ./src/RTC/RTCP/FeedbackPs.cpp \
    ./src/RTC/RTCP/FeedbackPsAfb.cpp \
    ./src/RTC/RTCP/FeedbackPsFir.cpp \
    ./src/RTC/RTCP/FeedbackPsLei.cpp \
    ./src/RTC/RTCP/FeedbackPsPli.cpp \
    ./src/RTC/RTCP/FeedbackPsRemb.cpp \
    ./src/RTC/RTCP/FeedbackPsRpsi.cpp \
    ./src/RTC/RTCP/FeedbackPsSli.cpp \
    ./src/RTC/RTCP/FeedbackPsTst.cpp \
    ./src/RTC/RTCP/FeedbackPsVbcm.cpp \
    ./src/RTC/RTCP/FeedbackRtp.cpp \
    ./src/RTC/RTCP/FeedbackRtpEcn.cpp \
    ./src/RTC/RTCP/FeedbackRtpNack.cpp \
    ./src/RTC/RTCP/FeedbackRtpSrReq.cpp \
    ./src/RTC/RTCP/FeedbackRtpTllei.cpp \
    ./src/RTC/RTCP/FeedbackRtpTmmb.cpp \
    ./src/RTC/RTCP/FeedbackRtpTransport.cpp \
    ./src/RTC/RTCP/Packet.cpp \
    ./src/RTC/RTCP/ReceiverReport.cpp \
    ./src/RTC/RTCP/Sdes.cpp \
    ./src/RTC/RTCP/SenderReport.cpp \
    ./src/RTC/RTCP/XR.cpp \
    ./src/RTC/RTCP/XrDelaySinceLastRr.cpp \
    ./src/RTC/RTCP/XrReceiverReferenceTime.cpp \
    ./src/RTC/RtpDictionaries/Media.cpp \
    ./src/RTC/RtpDictionaries/Parameters.cpp \
    ./src/RTC/RtpDictionaries/RtcpFeedback.cpp \
    ./src/RTC/RtpDictionaries/RtcpParameters.cpp \
    ./src/RTC/RtpDictionaries/RtpCodecMimeType.cpp \
    ./src/RTC/RtpDictionaries/RtpCodecParameters.cpp \
    ./src/RTC/RtpDictionaries/RtpEncodingParameters.cpp \
    ./src/RTC/RtpDictionaries/RtpHeaderExtensionParameters.cpp \
    ./src/RTC/RtpDictionaries/RtpHeaderExtensionUri.cpp \
    ./src/RTC/RtpDictionaries/RtpParameters.cpp \
    ./src/RTC/RtpDictionaries/RtpRtxParameters.cpp \
    ./src/RTC/RtpListener.cpp \
    ./src/RTC/RtpObserver.cpp \
    ./src/RTC/RtpPacket.cpp \
    ./src/RTC/RtpProbationGenerator.cpp \
	./src/RTC/RtpRetransmissionBuffer.cpp \
    ./src/RTC/RtpStream.cpp \
    ./src/RTC/RtpStreamRecv.cpp \
    ./src/RTC/RtpStreamSend.cpp \
    ./src/RTC/RtxStream.cpp \
    ./src/RTC/SctpAssociation.cpp \
    ./src/RTC/SctpDictionaries/SctpStreamParameters.cpp \
    ./src/RTC/SctpListener.cpp \
    ./src/RTC/SenderBandwidthEstimator.cpp \
    ./src/RTC/SeqManager.cpp \
    ./src/RTC/Shared.cpp \
    ./src/RTC/SimpleConsumer.cpp \
    ./src/RTC/SimulcastConsumer.cpp \
    ./src/RTC/SrtpSession.cpp \
    ./src/RTC/StunPacket.cpp \
    ./src/RTC/SvcConsumer.cpp \
    ./src/RTC/TcpConnection.cpp \
    ./src/RTC/TcpServer.cpp \
    ./src/RTC/Transport.cpp \
    ./src/RTC/TransportCongestionControlClient.cpp \
    ./src/RTC/TransportCongestionControlServer.cpp \
    ./src/RTC/TransportTuple.cpp \
    ./src/RTC/TrendCalculator.cpp \
    ./src/RTC/UdpSocket.cpp \
    ./src/RTC/WebRtcServer.cpp \
    ./src/RTC/WebRtcTransport.cpp \
    ./src/sdp/ortc.cpp \
    ./src/sdp/scalabilityMode.cpp \
    ./src/sdp/UtilsC.cpp \
    ./src/Settings.cpp \
    ./src/Utils/Crypto.cpp \
    ./src/Utils/File.cpp \
    ./src/Utils/IP.cpp \
    ./src/Utils/String.cpp \
    ./src/Worker.cpp \
    ./UvThread.cpp
	


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
