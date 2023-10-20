# Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
#  

TARGET = libwebrtc_ms_base
C++ := g++
CC := gcc
AR := ar
RANLIB := ranlib
INCLUDE_FLAG = -include
INCPATH = -I./ -I../../include -I../abseil-cpp -I../json/single_include -I../json/single_include/nlohmann
INCPATH += -I../../../openssl/include -I../../../libuv/include
CCFLAGS += -Wall -fPIC -D_GNU_SOURCE -fvisibility=hidden -DNDEBUG -O2 -DWEBRTC_POSIX -DWEBRTC_LINUX -DMS_LITTLE_ENDIAN -DLOGGING=1 -D__STDC_FORMAT_MACROS 


# EACH DIRECTORY WITH HEADERS MUST BE APPENDED IN THIS MANNER TO THE CCFLAGS
C++FLAGS = $(CCFLAGS)

INSTALL_DIR = ../../../../out/Linux_common

CFILES = \

CCFILES= \
    ./api/network_state_predictor.cc \
    ./api/transport/bitrate_settings.cc \
    ./api/transport/field_trial_based_config.cc \
    ./api/transport/goog_cc_factory.cc \
    ./api/transport/network_types.cc \
    ./api/units/data_rate.cc \
    ./api/units/data_size.cc \
    ./api/units/frequency.cc \
    ./api/units/timestamp.cc \
    ./api/units/time_delta.cc \
    ./call/rtp_transport_controller_send.cc \
    ./modules/bitrate_controller/loss_based_bandwidth_estimation.cc \
    ./modules/bitrate_controller/send_side_bandwidth_estimation.cc \
    ./modules/congestion_controller/goog_cc/acknowledged_bitrate_estimator.cc \
    ./modules/congestion_controller/goog_cc/alr_detector.cc \
    ./modules/congestion_controller/goog_cc/bitrate_estimator.cc \
    ./modules/congestion_controller/goog_cc/congestion_window_pushback_controller.cc \
    ./modules/congestion_controller/goog_cc/delay_based_bwe.cc \
    ./modules/congestion_controller/goog_cc/goog_cc_network_control.cc \
    ./modules/congestion_controller/goog_cc/link_capacity_estimator.cc \
    ./modules/congestion_controller/goog_cc/median_slope_estimator.cc \
    ./modules/congestion_controller/goog_cc/probe_bitrate_estimator.cc \
    ./modules/congestion_controller/goog_cc/probe_controller.cc \
    ./modules/congestion_controller/goog_cc/trendline_estimator.cc \
    ./modules/congestion_controller/rtp/control_handler.cc \
    ./modules/congestion_controller/rtp/send_time_history.cc \
    ./modules/congestion_controller/rtp/transport_feedback_adapter.cc \
    ./modules/pacing/bitrate_prober.cc \
    ./modules/pacing/interval_budget.cc \
    ./modules/pacing/paced_sender.cc \
    ./modules/remote_bitrate_estimator/aimd_rate_control.cc \
    ./modules/remote_bitrate_estimator/bwe_defines.cc \
    ./modules/remote_bitrate_estimator/inter_arrival.cc \
    ./modules/remote_bitrate_estimator/overuse_detector.cc \
    ./modules/remote_bitrate_estimator/overuse_estimator.cc \
    ./modules/remote_bitrate_estimator/remote_bitrate_estimator_abs_send_time.cc \
    ./modules/rtp_rtcp/include/rtp_rtcp_defines.cc \
    ./rtc_base/experiments/alr_experiment.cc \
    ./rtc_base/experiments/field_trial_parser.cc \
    ./rtc_base/experiments/field_trial_units.cc \
    ./rtc_base/experiments/rate_control_settings.cc \
    ./rtc_base/network/sent_packet.cc \
    ./rtc_base/rate_statistics.cc \
    ./system_wrappers/source/field_trial.cc 

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
