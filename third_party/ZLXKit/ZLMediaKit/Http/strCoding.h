﻿/*
 * Copyright (c) 2016 The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/xia-chu/ZLMediaKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef SRC_HTTP_STRCODING_H_
#define SRC_HTTP_STRCODING_H_

#include <iostream>
#include <string>
#include <cstdint>
namespace mediakit {

class strCoding {
public:
    static std::string UrlEncode(const std::string &str); //urlutf8 编码
    static std::string UrlDecode(const std::string &str); //urlutf8解码
#if defined(_WIN32)
    static std::string UTF8ToGB2312(const std::string &str);//utf_8转为gb2312
    static std::string GB2312ToUTF8(const std::string &str); //gb2312 转utf_8
#endif//defined(_WIN32)
private:
    strCoding(void);
    virtual ~strCoding(void);
};

} /* namespace mediakit */

#endif /* SRC_HTTP_STRCODING_H_ */
