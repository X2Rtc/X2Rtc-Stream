# Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
#  

TARGET = libwebrtc_ms_media
C++ := g++
CC := gcc
AR := ar
RANLIB := ranlib
INCLUDE_FLAG = -include
INCPATH = -I./ -I../../third_party/MediaSoup/include -I../../third_party/MediaSoup/deps/abseil-cpp -I../../third_party/MediaSoup/deps/libwebrtc -I../../third_party/openssl/include
CCFLAGS += -Wall -fPIC -D_GNU_SOURCE -fvisibility=hidden -DNDEBUG -O2 -DWEBRTC_POSIX -DWEBRTC_LINUX -DMS_LITTLE_ENDIAN -DLOGGING=1 -D__STDC_FORMAT_MACROS -DWEBRTC_APM_DEBUG_DUMP=0


# EACH DIRECTORY WITH HEADERS MUST BE APPENDED IN THIS MANNER TO THE CCFLAGS
C++FLAGS = $(CCFLAGS)

INSTALL_DIR = ../../out/Linux_common

CFILES = \
	./common_audio/signal_processing/resample.c \
    ./common_audio/signal_processing/resample_48khz.c \
    ./common_audio/signal_processing/resample_by_2.c \
    ./common_audio/signal_processing/resample_by_2_internal.c \
    ./common_audio/signal_processing/resample_fractional.c 

CCFILES= \
	./api/audio/audio_frame.cc \
    ./api/audio/channel_layout.cc \
    ./api/audio_codecs/audio_decoder.cc \
    ./api/audio_codecs/audio_format.cc \
    ./api/media_types.cc \
    ./api/rtp_headers.cc \
    ./api/rtp_packet_info.cc \
    ./api/rtp_parameters.cc \
    ./api/video/color_space.cc \
    ./api/video/encoded_frame_v.cc \
    ./api/video/encoded_image.cc \
    ./api/video/hdr_metadata.cc \
    ./api/video/video_content_type.cc \
    ./api/video/video_frame.cc \
    ./api/video/video_frame_buffer.cc \
    ./api/video/video_timing.cc \
    ./api/video_codecs/video_codec.cc \
    ./call/syncable.cc \
    ./common_audio/audio_util.cc \
    ./common_audio/resampler/push_resampler.cc \
    ./common_audio/resampler/push_sinc_resampler.cc \
    ./common_audio/resampler/resampler.cc \
    ./common_audio/resampler/sinc_resampler.cc \
    ./common_audio/resampler/sinc_resampler_sse.cc \
    ./common_audio/resampler/sinusoidal_linear_chirp_source.cc \
    ./common_video/generic_frame_descriptor/generic_frame_info.cc \
    ./common_video/h264/h264_bitstream_parser.cc \
    ./common_video/h264/h264_common.cc \
    ./common_video/h264/pps_parser.cc \
    ./common_video/h264/sps_parser.cc \
    ./common_video/h264/sps_vui_rewriter.cc \
	./common_video/h265/h265_bitstream_parser.cc \
    ./common_video/h265/h265_common.cc \
    ./common_video/h265/h265_pps_parser.cc \
    ./common_video/h265/h265_sps_parser.cc \
    ./common_video/h265/h265_vps_parser.cc \
    ./media/base/h264_profile_level_id.cc \
    ./media/base/media_constants.cc \
    ./modules/audio_processing/agc2/fixed_digital_level_estimator.cc \
    ./modules/audio_processing/agc2/interpolated_gain_curve.cc \
    ./modules/audio_processing/agc2/limiter.cc \
    ./modules/audio_processing/logging/apm_data_dumper.cc \
    ./modules/include/module_common_types.cc \
    ./modules/rtp_rtcp/source/absolute_capture_time_receiver.cc \
    ./modules/rtp_rtcp/source/remote_ntp_time_estimator.cc \
    ./modules/rtp_rtcp/source/rtp_depacketizer_av1.cc \
    ./modules/rtp_rtcp/source/rtp_dependency_descriptor_extension.cc \
    ./modules/rtp_rtcp/source/rtp_dependency_descriptor_reader.cc \
    ./modules/rtp_rtcp/source/rtp_dependency_descriptor_writer.cc \
    ./modules/rtp_rtcp/source/rtp_format.cc \
    ./modules/rtp_rtcp/source/rtp_format_h264.cc \
	./modules/rtp_rtcp/source/rtp_format_h265.cc \
    ./modules/rtp_rtcp/source/rtp_format_video_generic.cc \
    ./modules/rtp_rtcp/source/rtp_format_vp8.cc \
    ./modules/rtp_rtcp/source/rtp_format_vp9.cc \
    ./modules/rtp_rtcp/source/rtp_generic_frame_descriptor.cc \
    ./modules/rtp_rtcp/source/rtp_generic_frame_descriptor_extension.cc \
    ./modules/rtp_rtcp/source/rtp_header_extensions.cc \
    ./modules/rtp_rtcp/source/rtp_header_extension_map.cc \
    ./modules/rtp_rtcp/source/rtp_packet.cc \
    ./modules/rtp_rtcp/source/rtp_packetizer_av1.cc \
    ./modules/rtp_rtcp/source/rtp_packet_received.cc \
    ./modules/rtp_rtcp/source/rtp_packet_to_send.cc \
    ./modules/rtp_rtcp/source/rtp_utility.cc \
    ./modules/rtp_rtcp/source/rtp_video_header.cc \
    ./modules/video_coding/encoded_frame.cc \
    ./modules/video_coding/frame_object.cc \
    ./modules/video_coding/h264_sprop_parameter_sets.cc \
    ./modules/video_coding/h264_sps_pps_tracker.cc \
	./modules/video_coding/h265_vps_sps_pps_tracker.cc \
    ./modules/video_coding/include/video_codec_interface.cc \
    ./modules/video_coding/packet_buffer.cc \
    ./modules/video_coding/rtp_frame_reference_finder.cc \
    ./modules/video_coding/unique_timestamp_counter.cc \
    ./rtc_base/bit_buffer.cc \
    ./rtc_base/byte_buffer.cc \
    ./rtc_base/checks.cc \
    ./rtc_base/copy_on_write_buffer.cc \
    ./rtc_base/critical_section.cc \
    ./rtc_base/helpers.cc \
    ./rtc_base/logging.cc \
	./rtc_base/memory/aligned_malloc.cc \
    ./rtc_base/platform_thread_types.cc \
    ./rtc_base/strings/string_builder.cc \
    ./rtc_base/string_encode.cc \
    ./rtc_base/string_to_number.cc \
    ./rtc_base/string_utils.cc \
    ./rtc_base/synchronization/rw_lock_posix.cc \
    ./rtc_base/synchronization/rw_lock_wrapper.cc \
    ./rtc_base/third_party/base64/base64.cc \
    ./rtc_base/time_utils.cc \
    ./rtc_base/unique_id_generator.cc \
    ./rtc_base/zero_memory.cc \
    ./system_wrappers/source/clock.cc \
    ./system_wrappers/source/metrics.cc \
    ./system_wrappers/source/rtp_to_ntp_estimator.cc \
    ./video/rtp_streams_synchronizer.cc \
    ./video/stream_synchronization.cc 

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
