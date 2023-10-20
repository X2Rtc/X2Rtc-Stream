# Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
#  

TARGET = libX2RtcAudCodec
C++ := g++
CC := gcc
AR := ar
RANLIB := ranlib
INCLUDE_FLAG = -include
INCPATH = -I./ -I./webrtc -I../ -I../third_party -I../third_party/jthread/src -I../third_party/opus/src/include -I../third_party/faac-1.28/include -I../third_party/faad2-2.7/include -I../third_party/abseil-cpp
INCLUDE_FLAG = -include
LIBPATH = -lpthread -L../out/Linux_common -lfaac -lfaad -lopus -ljthread -Wl,-no-whole-archive 
CCFLAGS += -Wall -fPIC -D_GNU_SOURCE -fvisibility=hidden -DNDEBUG -O2 -DNDEBUG -O2 -w -c -D_CRT_SECURE_NO_WARNINGS -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -DMS_LITTLE_ENDIAN -DWEBRTC_POSIX -DWEBRTC_LINUX -DNOMINMAX -DWEBRTC_APM_DEBUG_DUMP=0
LFLAGS = -w -Wl,--export-dynamic -Wl,-whole-archive -shared 

# EACH DIRECTORY WITH HEADERS MUST BE APPENDED IN THIS MANNER TO THE CCFLAGS
C++FLAGS = $(CCFLAGS)

INSTALL_DIR = ../out/Linux_common

CFILES = \
	./webrtc/common_audio/signal_processing/auto_correlation.c \
    ./webrtc/common_audio/signal_processing/auto_corr_to_refl_coef.c \
    ./webrtc/common_audio/signal_processing/complex_bit_reverse.c \
    ./webrtc/common_audio/signal_processing/complex_fft.c \
    ./webrtc/common_audio/signal_processing/copy_set_operations.c \
    ./webrtc/common_audio/signal_processing/cross_correlation.c \
    ./webrtc/common_audio/signal_processing/division_operations.c \
	./webrtc/common_audio/signal_processing/downsample_fast.c \
    ./webrtc/common_audio/signal_processing/energy.c \
    ./webrtc/common_audio/signal_processing/filter_ar.c \
    ./webrtc/common_audio/signal_processing/filter_ar_fast_q12.c \
    ./webrtc/common_audio/signal_processing/filter_ma_fast_q12.c \
    ./webrtc/common_audio/signal_processing/get_hanning_window.c \
    ./webrtc/common_audio/signal_processing/get_scaling_square.c \
    ./webrtc/common_audio/signal_processing/ilbc_specific_functions.c \
    ./webrtc/common_audio/signal_processing/levinson_durbin.c \
    ./webrtc/common_audio/signal_processing/lpc_to_refl_coef.c \
    ./webrtc/common_audio/signal_processing/min_max_operations.c \
    ./webrtc/common_audio/signal_processing/randomization_functions.c \
    ./webrtc/common_audio/signal_processing/real_fft.c \
    ./webrtc/common_audio/signal_processing/refl_coef_to_lpc.c \
    ./webrtc/common_audio/signal_processing/resample.c \
    ./webrtc/common_audio/signal_processing/resample_48khz.c \
    ./webrtc/common_audio/signal_processing/resample_by_2.c \
    ./webrtc/common_audio/signal_processing/resample_by_2_internal.c \
    ./webrtc/common_audio/signal_processing/resample_fractional.c \
    ./webrtc/common_audio/signal_processing/splitting_filter.c \
    ./webrtc/common_audio/signal_processing/spl_init.c \
    ./webrtc/common_audio/signal_processing/spl_inl.c \
    ./webrtc/common_audio/signal_processing/spl_sqrt.c \
    ./webrtc/common_audio/signal_processing/sqrt_of_one_minus_x_squared.c \
    ./webrtc/common_audio/signal_processing/vector_scaling_operations.c \
	./webrtc/modules/audio_coding/codecs/g711/g711_interface.c \
    ./webrtc/modules/audio_coding/codecs/opus/opus_interface.c \
    ./webrtc/modules/third_party/g711/g711.c 
	

CCFILES = \
	./aacdecode.cc \
    ./aacencode.cc \
    ./webrtc/common_audio/audio_util.cc \
    ./webrtc/common_audio/resampler/push_resampler.cc \
    ./webrtc/common_audio/resampler/push_sinc_resampler.cc \
    ./webrtc/common_audio/resampler/resampler.cc \
    ./webrtc/common_audio/resampler/sinc_resampler.cc \
    ./webrtc/common_audio/resampler/sinc_resampler_sse.cc \
    ./webrtc/common_audio/resampler/sinusoidal_linear_chirp_source.cc \
    ./webrtc/common_audio/signal_processing/dot_product_with_scale.cc \
    ./webrtc/modules/audio_coding/acm2/acm_resampler.cc \
    ./webrtc/rtc_base/checks.cc \
    ./webrtc/rtc_base/critical_section.cc \
    ./webrtc/rtc_base/logging.cc \
    ./webrtc/rtc_base/memory/aligned_malloc.cc \
    ./webrtc/rtc_base/platform_thread_types.cc \
    ./webrtc/rtc_base/strings/string_builder.cc \
    ./webrtc/rtc_base/string_encode.cc \
    ./webrtc/rtc_base/string_to_number.cc \
    ./webrtc/rtc_base/string_utils.cc \
    ./webrtc/rtc_base/time_utils.cc \
    ./webrtc/system_wrappers/source/cpu_features.cc 

CPPFILES = \
	./X2RtcAudCodec.cpp \
    ./X2AudioUtil.cpp 
    

	


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
