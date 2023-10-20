# Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
#  

TARGET = libsrtp
C++ := g++
CC := gcc
AR := ar
RANLIB := ranlib
INCLUDE_FLAG = -include
INCPATH = -I./ -I./config -I./include -I./crypto/include  -I../openssl/include
LINK = $(LINKER)
CCFLAGS += -Wall -fPIC -D_GNU_SOURCE -fvisibility=hidden -DNDEBUG -O2 -Wno-unused-variable -DHAVE_CONFIG_H -DHAVE_STDLIB_H -DHAVE_STRING_H -DOPENSSL -DCPU_CISC
CCFLAGS += -DHAVE_INT16_T -DHAVE_INT32_T -DHAVE_INT8_T -DHAVE_UINT16_T -DHAVE_UINT32_T -DHAVE_UINT64_T -DHAVE_UINT8_T -DHAVE_STDINT_H -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H -DHAVE_ARPA_INET_H -DHAVE_UNISTD_H 

# EACH DIRECTORY WITH HEADERS MUST BE APPENDED IN THIS MANNER TO THE CCFLAGS
C++FLAGS = $(CCFLAGS)

INSTALL_DIR = ../../out/Linux_common

CFILES = ./srtp/ekt.c \
            ./srtp/srtp.c \
			./crypto/cipher/aes_gcm_ossl.c \
			./crypto/cipher/aes_icm_ossl.c \
            ./crypto/cipher/cipher.c \
            ./crypto/cipher/null_cipher.c \
            ./crypto/hash/auth.c \
			./crypto/hash/hmac_ossl.c \
            ./crypto/hash/null_auth.c \
            ./crypto/kernel/alloc.c \
            ./crypto/kernel/crypto_kernel.c \
            ./crypto/kernel/err.c \
            ./crypto/kernel/key.c \
            ./crypto/math/datatypes.c \
            ./crypto/math/stat.c \
            ./crypto/replay/rdb.c \
            ./crypto/replay/rdbx.c \
            ./crypto/replay/ut_sim.c
			
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
