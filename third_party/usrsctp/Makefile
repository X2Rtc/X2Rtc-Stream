# Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
#  

TARGET = libusrsctp
C++ := g++
CC := gcc
AR := ar
RANLIB := ranlib
INCLUDE_FLAG = -include
INCPATH = -I./ -I./usrsctplib/usrsctplib/ -I./usrsctplib/usrsctplib/netinet/ -I../openssl/include
CCFLAGS += -Wall -fPIC -D_GNU_SOURCE -fvisibility=hidden -DNDEBUG -O2 -DSCTP_PROCESS_LEVEL_LOCKS -DSCTP_SIMPLE_ALLOCATOR -D__Userspace__ -DSCTP_USE_OPENSSL_SHA1 -D__Userspace_os_Linux

# EACH DIRECTORY WITH HEADERS MUST BE APPENDED IN THIS MANNER TO THE CCFLAGS
C++FLAGS = $(CCFLAGS)

INSTALL_DIR = ../../out/Linux_common

CFILES = ./usrsctplib/usrsctplib/netinet/sctp_asconf.c \
        ./usrsctplib/usrsctplib/netinet/sctp_auth.c \
        ./usrsctplib/usrsctplib/netinet/sctp_bsd_addr.c \
        ./usrsctplib/usrsctplib/netinet/sctp_callout.c \
        ./usrsctplib/usrsctplib/netinet/sctp_cc_functions.c \
        ./usrsctplib/usrsctplib/netinet/sctp_crc32.c \
        ./usrsctplib/usrsctplib/netinet/sctp_indata.c \
        ./usrsctplib/usrsctplib/netinet/sctp_input.c \
        ./usrsctplib/usrsctplib/netinet/sctp_output.c \
        ./usrsctplib/usrsctplib/netinet/sctp_pcb.c \
        ./usrsctplib/usrsctplib/netinet/sctp_peeloff.c \
        ./usrsctplib/usrsctplib/netinet/sctp_sha1.c \
        ./usrsctplib/usrsctplib/netinet/sctp_ss_functions.c \
        ./usrsctplib/usrsctplib/netinet/sctp_sysctl.c \
        ./usrsctplib/usrsctplib/netinet/sctp_timer.c \
        ./usrsctplib/usrsctplib/netinet/sctp_userspace.c \
        ./usrsctplib/usrsctplib/netinet/sctp_usrreq.c \
        ./usrsctplib/usrsctplib/netinet/sctputil.c \
        ./usrsctplib/usrsctplib/netinet6/sctp6_usrreq.c \
        ./usrsctplib/usrsctplib/user_environment.c \
        ./usrsctplib/usrsctplib/user_mbuf.c \
        ./usrsctplib/usrsctplib/user_recv_thread.c \
        ./usrsctplib/usrsctplib/user_socket.c 

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
