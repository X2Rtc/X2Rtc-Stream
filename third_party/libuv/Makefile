# Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
#  

TARGET = libuv
C++ := g++
CC := gcc
AR := ar
RANLIB := ranlib
INCLUDE_FLAG = -include
INCPATH = -I./ -I./include -I./src
INCLUDE_FLAG = -include
LIBPATH = -lpthread -Wl,-no-whole-archive 
CCFLAGS += -Wall -fPIC -D_GNU_SOURCE -fvisibility=hidden -DNDEBUG -O2 -DNDEBUG -O2 -w -c
LFLAGS = -w -Wl,--export-dynamic -Wl,-whole-archive -shared 

# EACH DIRECTORY WITH HEADERS MUST BE APPENDED IN THIS MANNER TO THE CCFLAGS
C++FLAGS = $(CCFLAGS)

INSTALL_DIR = ../../out/Linux_common

CFILES = \
	./src/fs-poll.c \
	./src/idna.c \
	./src/inet.c \
	./src/random.c \
	./src/strscpy.c \
	./src/threadpool.c \
	./src/timer.c \
	./src/uv-data-getter-setters.c \
	./src/uv-common.c \
	./src/version.c \
	./src/unix/async.c \
	./src/unix/core.c \
	./src/unix/dl.c \
	./src/unix/fs.c \
	./src/unix/getaddrinfo.c \
	./src/unix/getnameinfo.c \
	./src/unix/loop-watcher.c \
	./src/unix/loop.c \
	./src/unix/pipe.c \
	./src/unix/poll.c \
	./src/unix/process.c \
	./src/unix/random-devurandom.c \
	./src/unix/signal.c \
	./src/unix/stream.c \
	./src/unix/tcp.c \
	./src/unix/thread.c \
	./src/unix/tty.c \
	./src/unix/udp.c \
	./src/unix/linux-core.c \
	./src/unix/linux-inotify.c \
	./src/unix/linux-syscalls.c \
	./src/unix/procfs-exepath.c \
	./src/unix/proctitle.c \
	./src/unix/random-getrandom.c \
	./src/unix/random-sysctl-linux.c
	

CCFILES = \

CPPFILES = \

    

	


OUT_OBJ = ./out/Obj_common

CFILES_OBJ := $(patsubst %.c,%.o,$(CFILES))

CC_FILES_OBJ := $(patsubst %.cc,%.o,$(CCFILES))

CPP_FILES_OBJ := $(patsubst %.cpp,%.o,$(CPPFILES))
		
all: $(TARGET).so 

$(TARGET).so: $(CCFILES:.cc=.o) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)
	$(CC)  $(LFLAGS) $(CFILES_OBJ) $(CPP_FILES_OBJ) $(CC_FILES_OBJ) -o $(OUT_OBJ)/$(TARGET).so $(LIBPATH)

install: 
	install -m 664 $(OUT_OBJ)/$(TARGET).so $(INSTALL_DIR)
	
clean:
	rm -f $(OUT_OBJ)/$(TARGET).so $(CFILES_OBJ) $(CC_FILES_OBJ) $(CPP_FILES_OBJ) 

.SUFFIXES: .cc .cpp .c .o

.c.o:
	$(CC) -c -o $*.o $(DEFINES) -std=c99 $(CCFLAGS) $*.c $(INCPATH)
	
.cc.o:
	$(C++) -c -o $*.o $(DEFINES) -std=c++11 $(C++FLAGS) -fpermissive $*.cc $(INCPATH)
	
.cpp.o:
	$(C++) -c -o $*.o $(DEFINES) -std=c++11 $(C++FLAGS) -fpermissive $*.cpp $(INCPATH) 
