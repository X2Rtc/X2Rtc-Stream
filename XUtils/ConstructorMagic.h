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
#ifndef X2RTC_BASE_CONSTRUCTOR_MAGIC_H_
#define X2RTC_BASE_CONSTRUCTOR_MAGIC_H_

// Put this in the declarations for a class to be unassignable.
#define X2RTC_DISALLOW_ASSIGN(TypeName) \
  TypeName& operator=(const TypeName&) = delete

// A macro to disallow the copy constructor and operator= functions. This should
// be used in the declarations for a class.
#define X2RTC_DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;          \
  X2RTC_DISALLOW_ASSIGN(TypeName)

// A macro to disallow all the implicit constructors, namely the default
// constructor, copy constructor and operator= functions.
//
// This should be used in the declarations for a class that wants to prevent
// anyone from instantiating it. This is especially useful for classes
// containing only static methods.
#define X2RTC_DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
  TypeName() = delete;                               \
  X2RTC_DISALLOW_COPY_AND_ASSIGN(TypeName)

#endif  // X2RTC_BASE_CONSTRUCTOR_MAGIC_H_
