/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "rtc_base/string_utils.h"

namespace rtc {

	bool memory_check(const void* memory, int c, size_t count) {
		const char* char_memory = static_cast<const char*>(memory);
		char char_c = static_cast<char>(c);
		for (size_t i = 0; i < count; ++i) {
			if (char_memory[i] != char_c) {
				return false;
			}
		}
		return true;
	}

	bool string_match(const char* target, const char* pattern) {
		while (*pattern) {
			if (*pattern == '*') {
				if (!*++pattern) {
					return true;
				}
				while (*target) {
					if ((toupper(*pattern) == toupper(*target))
						&& string_match(target + 1, pattern + 1)) {
						return true;
					}
					++target;
				}
				return false;
			}
			else {
				if (toupper(*pattern) != toupper(*target)) {
					return false;
				}
				++target;
				++pattern;
			}
		}
		return !*target;
	}

size_t strcpyn(char* buffer,
               size_t buflen,
               const char* source,
               size_t srclen /* = SIZE_UNKNOWN */) {
  if (buflen <= 0)
    return 0;

  if (srclen == SIZE_UNKNOWN) {
    srclen = strlen(source);
  }
  if (srclen >= buflen) {
    srclen = buflen - 1;
  }
  memcpy(buffer, source, srclen);
  buffer[srclen] = 0;
  return srclen;
}

static const char kWhitespace[] = " \n\r\t";

std::string string_trim(const std::string& s) {
  std::string::size_type first = s.find_first_not_of(kWhitespace);
  std::string::size_type last = s.find_last_not_of(kWhitespace);

  if (first == std::string::npos || last == std::string::npos) {
    return std::string("");
  }

  return s.substr(first, last - first + 1);
}

std::string ToHex(const int i) {
  char buffer[50];
  snprintf(buffer, sizeof(buffer), "%x", i);

  return std::string(buffer);
}

std::string LeftPad(char padding, unsigned length, std::string s) {
  if (s.length() >= length)
    return s;
  return std::string(length - s.length(), padding) + s;
}

}  // namespace rtc
