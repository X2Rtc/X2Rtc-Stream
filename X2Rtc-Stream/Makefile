# Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
#  

TARGET = X2RtcStream
C++ = g++
CC = gcc
INCLUDE_FLAG = -include
INCPATH = -I./ -I../ -I../XUtils -I../X2Rtc-AudCodec -I../third_party -I../third_party/libuv/include -I../third_party/jthread/src
LIBPATH = -lpthread -L../out/Linux_common -lX2ModFlv -lX2ModRtc -lX2ModRtmp -lX2ModRtp -lX2ModSrt -lX2NetUv -lX2UvBase 
LIBPATH += -lMediaSoup -lwebrtc_ms_base -lwebrtc_ms_media -lZLXKit -lX2RtcAudCodec 
LIBPATH += -labseil-cpp -lHttpParser -ljthread -lflv -lmpeg -lsrtp -luv -lusrsctp
LIBPATH += -L../third_lib/x64/linux -lsrt -lssl -lcrypto

CCFLAGS += -Wall -fPIC -D_GNU_SOURCE -fvisibility=hidden -DNDEBUG -O2 -DWEBRTC_POSIX -DWEBRTC_LINUX -DDEPS_UV -D__STDC_FORMAT_MACROS -D__STDC_CONSTANT_MACROS
LFLAGS	= -w -lrt -ldl

# EACH DIRECTORY WITH HEADERS MUST BE APPENDED IN THIS MANNER TO THE CCFLAGS
C++FLAGS = $(CCFLAGS)

INSTALL_DIR = ../out/Linux_common

CFILES = 

CPPFILES = \
	../XUtils/X2Config.cpp \
    ../XUtils/X2RtcEvent.cpp \
    ../XUtils/X2RtcLog.cpp \
    ../XUtils/XUtil.cpp \
	../XUtils/X2CodecType.cpp \
    ./main.cpp \
    ./X2MediaDispatch.cpp \
	./X2MediaRtn.cpp \
    ./X2NetPeer.cpp \
    ./X2NetProcess.cpp \
    ./X2ProtoHandler.cpp \
    ./X2ProxyImpl.cpp
		
CCFILES = \
		

OUT_OBJ = ./out/Obj_common

CFILES_OBJ := $(patsubst %.c,%.o,$(CFILES))

CC_FILES_OBJ := $(patsubst %.cc,%.o,$(CCFILES))

CPP_FILES_OBJ := $(patsubst %.cpp,%.o,$(CPPFILES))

all: $(TARGET)

$(TARGET): $(CFILES:.c=.o) $(CCFILES:.cc=.o) $(CPPFILES:.cpp=.o)
	$(C++)  $(CFILES:.c=.o) $(CCFILES:.cc=.o) $(CPPFILES:.cpp=.o) -o $(OUT_OBJ)/$(TARGET) $(LIBPATH) $(LFLAGS)

install: 
	install -m 664 $(OUT_OBJ)/$(TARGET) $(INSTALL_DIR)

clean:
	rm -f $(OUT_OBJ)/$(TARGET) $(CFILES:.c=.o) $(CCFILES:.cc=.o) $(CPPFILES:.cpp=.o)

.SUFFIXES: .cpp .cc .c .o

.cpp.o:
	$(C++) -c -o $*.o $(DEFINES) -std=c++11 $(C++FLAGS) -fpermissive $*.cpp $(INCPATH)
	
.cc.o:
	$(C++) -c -o $*.o $(DEFINES) -std=c++11 $(C++FLAGS) -fpermissive $*.cc $(INCPATH)

.c.o:
	$(CC) -c -o $*.o $(DEFINES) -std=c99 $(CCFLAGS) $*.c $(INCPATH) 
