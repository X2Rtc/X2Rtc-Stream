# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.cpprg/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
################################################################
# Ucc Jni
LOCAL_PATH := $(call my-dir)
RTMP := ../
include $(CLEAR_VARS)

LOCAL_MODULE    := libusrsctp
			
LOCAL_SRC_FILES := \
		./usrsctplib/usrsctplib/netinet/sctp_asconf.c \
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
## 
## Widows (call host-path,/cygdrive/path/to/your/file/libstlport_shared.so) 	
#		   
#LOCAL_LDLIBS := -llog
LOCAL_CFLAGS := -w -Wno-unused-function -DSCTP_PROCESS_LEVEL_LOCKS -DSCTP_SIMPLE_ALLOCATOR -DSCTP_USE_OPENSSL_SHA1 -D__Userspace__ -D__Userspace_os_Linux -D_GNU_SOURCE
#

LOCAL_C_INCLUDES += $(NDK_STL_INC) \
		$(LOCAL_PATH)/usrsctplib/usrsctplib \
		$(LOCAL_PATH)/usrsctplib/usrsctplib/netinet \
		$(LOCAL_PATH)/../boringssl/src/include

include $(BUILD_STATIC_LIBRARY)