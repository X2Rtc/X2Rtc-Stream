# Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
#  

TARGET = libX2NetUv
C++ := g++
CC := gcc
AR := ar
RANLIB := ranlib
INCLUDE_FLAG = -include
INCPATH = -I./ -I../ -I../X2UV-Base -I../../third_party/abseil-cpp -I../../XUtils -I../../third_party -I../../third_party/jthread/src -I../../third_party/libuv/include -I../../third_party/openssl/include
CCFLAGS += -Wall -fPIC -D_GNU_SOURCE -fvisibility=hidden -DNDEBUG -O2 -DWEBRTC_POSIX -DMS_LITTLE_ENDIAN -DLOGGING=1 -D__STDC_FORMAT_MACROS 


# EACH DIRECTORY WITH HEADERS MUST BE APPENDED IN THIS MANNER TO THE CCFLAGS
C++FLAGS = $(CCFLAGS)

INSTALL_DIR = ../../out/Linux_common

CFILES = \

CCFILES= \

CPPFILES = \
	./X2HttpConnection.cpp \
	./X2HttpServer.cpp \
    ./X2NetTcpClient.cpp \
    ./X2NetTcpConnection.cpp \
    ./X2NetTcpProcess.cpp \
    ./X2NetUdpConnection.cpp \
    ./X2NetUdpProcess.cpp \
    ./X2TcpServer.cpp
	


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
