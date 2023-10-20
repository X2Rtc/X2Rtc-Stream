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

LOCAL_MODULE    := libsrtp
			
LOCAL_SRC_FILES := \
		./srtp/ekt.c \
		./srtp/srtp.c \
		./crypto/cipher/cipher.c \
		./crypto/cipher/null_cipher.c \
		./crypto/hash/auth.c \
		./crypto/hash/null_auth.c \
		./crypto/kernel/alloc.c \
		./crypto/kernel/crypto_kernel.c \
		./crypto/kernel/err.c \
		./crypto/kernel/key.c \
		./crypto/math/datatypes.c \
		./crypto/math/stat.c \
		./crypto/replay/rdb.c \
		./crypto/replay/rdbx.c \
		./crypto/replay/ut_sim.c \
		./crypto/cipher/aes_gcm_ossl.c \
		./crypto/cipher/aes_icm_ossl.c \
		./crypto/hash/hmac_ossl.c
	
## 
## Widows (call host-path,/cygdrive/path/to/your/file/libstlport_shared.so) 	
#		   
#LOCAL_LDLIBS := -llog
LOCAL_CFLAGS := -Wno-unused-variable -DHAVE_CONFIG_H -DHAVE_STDLIB_H -DHAVE_STRING_H -DOPENSSL -DCPU_CISC
LOCAL_CFLAGS += -DHAVE_INT16_T -DHAVE_INT32_T -DHAVE_INT8_T -DHAVE_UINT16_T -DHAVE_UINT32_T -DHAVE_UINT64_T
LOCAL_CFLAGS += -DHAVE_UINT8_T -DHAVE_STDINT_H -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H -DHAVE_ARPA_INET_H -DHAVE_UNISTD_H
#

LOCAL_C_INCLUDES += $(NDK_STL_INC) \
		$(LOCAL_PATH)/config \
		$(LOCAL_PATH)/include \
		$(LOCAL_PATH)/crypto/include \
		$(LOCAL_PATH)/../boringssl/src/include

include $(BUILD_STATIC_LIBRARY)