# Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
#  

TARGET = libfaad
C++ := g++
CC := gcc
AR := ar
RANLIB := ranlib
INCLUDE_FLAG = -include
INCPATH = -I./ -I./include -I./libfaad
LINK = $(LINKER)
CCFLAGS += -Wall -fPIC -D_GNU_SOURCE -fvisibility=hidden -DNDEBUG -O2 -Wno-unused-variable -ffast-math -fprefetch-loop-arrays -DFAAD_HAVE_CONFIG_H

# EACH DIRECTORY WITH HEADERS MUST BE APPENDED IN THIS MANNER TO THE CCFLAGS
C++FLAGS = $(CCFLAGS)

INSTALL_DIR = ../../out/Linux_common

CFILES = \
	./libfaad/bits.c \
	./libfaad/cfft.c \
	./libfaad/decoder.c \
	./libfaad/drc.c \
	./libfaad/drm_dec.c \
	./libfaad/error.c \
	./libfaad/filtbank.c \
	./libfaad/ic_predict.c \
	./libfaad/is.c \
	./libfaad/lt_predict.c \
	./libfaad/mdct.c \
	./libfaad/mp4.c \
	./libfaad/ms.c \
	./libfaad/output.c \
	./libfaad/pns.c \
	./libfaad/ps_dec.c \
	./libfaad/ps_syntax.c \
	./libfaad/pulse.c \
	./libfaad/specrec.c \
	./libfaad/syntax.c \
	./libfaad/tns.c \
	./libfaad/hcr.c \
	./libfaad/huffman.c \
	./libfaad/rvlc.c \
	./libfaad/ssr.c \
	./libfaad/ssr_fb.c \
	./libfaad/ssr_ipqf.c \
	./libfaad/common.c \
	./libfaad/sbr_dct.c \
	./libfaad/sbr_e_nf.c \
	./libfaad/sbr_fbt.c \
	./libfaad/sbr_hfadj.c \
	./libfaad/sbr_hfgen.c \
	./libfaad/sbr_huff.c \
	./libfaad/sbr_qmf.c \
	./libfaad/sbr_syntax.c \
	./libfaad/sbr_tf_grid.c \
	./libfaad/sbr_dec.c

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
