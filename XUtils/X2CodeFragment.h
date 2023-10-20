/*
*  Copyright (c) 2022 The X2RTC project authors. All Rights Reserved.
*
*  Please visit https://www.x2rtc.com for detail.
*
*  Author: Eric(eric@x2rtc.com)
*  Twitter: @X2rtc_cloud
*
* The GNU General Public License is a free, copyleft license for
* software and other kinds of works.
*
* The licenses for most software and other practical works are designed
* to take away your freedom to share and change the works.  By contrast,
* the GNU General Public License is intended to guarantee your freedom to
* share and change all versions of a program--to make sure it remains free
* software for all its users.  We, the Free Software Foundation, use the
* GNU General Public License for most of our software; it applies also to
* any other work released this way by its authors.  You can apply it to
* your programs, too.
* See the GNU LICENSE file for more info.
*/

#ifndef __X2_CODE_FRAGMENT_H__
#define __X2_CODE_FRAGMENT_H__

#include <stdlib.h>

#if _WIN32
// Win32 includes here
#include <Windows.h>
#endif

#ifdef __MacOSX__
#include <CoreFoundation/CFBundle.h>
#endif

class X2CodeFragment
{
    public:   
        X2CodeFragment(const char* inPath);
        ~X2CodeFragment();
        
        bool  IsValid() { return (fFragmentP != NULL); }
        void*   GetSymbol(const char* inSymbolName);
        
    private:
    
#ifdef _WIN32
        HMODULE fFragmentP;
#elif __MacOSX__
        CFBundleRef fFragmentP;
#else
        void*   fFragmentP;
#endif
};

#endif//__X2_CODE_FRAGMENT_H__
