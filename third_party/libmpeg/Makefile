# Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
#  

TARGET = libmpeg
C++ := g++
CC := gcc
AR := ar
RANLIB := ranlib
INCLUDE_FLAG = -include
INCPATH = -I./ -I./include
LINK = $(LINKER)
CCFLAGS += -Wall -fPIC -D_GNU_SOURCE -fvisibility=hidden -DNDEBUG -O2 -Wno-unused-variable -D_CRT_SECURE_NO_WARNINGS

# EACH DIRECTORY WITH HEADERS MUST BE APPENDED IN THIS MANNER TO THE CCFLAGS
C++FLAGS = $(CCFLAGS)

INSTALL_DIR = ../../out/Linux_common

CFILES = \
	./source/mpeg-crc32.c \
    ./source/mpeg-element-descriptor.c \
    ./source/mpeg-muxer.c \
    ./source/mpeg-pack-header.c \
    ./source/mpeg-packet.c \
    ./source/mpeg-pat.c \
    ./source/mpeg-pes.c \
    ./source/mpeg-pmt.c \
    ./source/mpeg-ps-dec.c \
    ./source/mpeg-ps-enc.c \
    ./source/mpeg-psd.c \
    ./source/mpeg-psm.c \
    ./source/mpeg-sdt.c \
    ./source/mpeg-system-header.c \
    ./source/mpeg-ts-dec.c \
    ./source/mpeg-ts-enc.c \
    ./source/mpeg-ts-h264.c \
    ./source/mpeg-ts-h265.c \
    ./source/mpeg-ts-h266.c \
    ./source/mpeg-util.c 

CCFILES = \

	
CPPFILES = \

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
