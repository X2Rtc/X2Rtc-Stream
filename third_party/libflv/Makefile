# Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
#  

TARGET = libflv
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
	./source/amf0.c \
    ./source/amf3.c \
    ./source/aom-av1.c \
    ./source/flv-demuxer-script.c \
    ./source/flv-demuxer.c \
    ./source/flv-header.c \
    ./source/flv-muxer.c \
    ./source/flv-parser.c \
    ./source/flv-reader.c \
    ./source/flv-writer.c \
    ./source/hevc-annexbtomp4.c \
    ./source/hevc-mp4toannexb.c \
    ./source/mp3-header.c \
    ./source/mpeg4-vvc.c \
    ./source/mpeg4-aac-asc.c \
    ./source/mpeg4-aac.c \
    ./source/mpeg4-annexbtomp4.c \
    ./source/mpeg4-avc.c \
    ./source/mpeg4-hevc.c \
    ./source/mpeg4-mp4toannexb.c \
    ./source/opus-head.c \
    ./source/riff-acm.c \
    ./source/vvc-annexbtomp4.c \
    ./source/vvc-mp4toannexb.c \
    ./source/webm-vpx.c 

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
