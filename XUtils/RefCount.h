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
#ifndef X2RTC_BASE_REF_COUNT_H_
#define X2RTC_BASE_REF_COUNT_H_

namespace x2rtc {

// Refcounted objects should implement the following informal interface:
//
// void AddRef() const ;
// RefCountReleaseStatus Release() const;
//
// You may access members of a reference-counted object, including the AddRef()
// and Release() methods, only if you already own a reference to it, or if
// you're borrowing someone else's reference. (A newly created object is a
// special case: the reference count is zero on construction, and the code that
// creates the object should immediately call AddRef(), bringing the reference
// count from zero to one, e.g., by constructing an x2rtc::scoped_refptr).
//
// AddRef() creates a new reference to the object.
//
// Release() releases a reference to the object; the caller now has one less
// reference than before the call. Returns kDroppedLastRef if the number of
// references dropped to zero because of this (in which case the object destroys
// itself). Otherwise, returns kOtherRefsRemained, to signal that at the precise
// time the caller's reference was dropped, other references still remained (but
// if other threads own references, this may of course have changed by the time
// Release() returns).
//
// The caller of Release() must treat it in the same way as a delete operation:
// Regardless of the return value from Release(), the caller mustn't access the
// object. The object might still be alive, due to references held by other
// users of the object, but the object can go away at any time, e.g., as the
// result of another thread calling Release().
//
// Calling AddRef() and Release() manually is discouraged. It's recommended to
// use x2rtc::scoped_refptr to manage all pointers to reference counted objects.
// Note that x2rtc::scoped_refptr depends on compile-time duck-typing; formally
// implementing the below RefCountInterface is not required.

enum class RefCountReleaseStatus { kDroppedLastRef, kOtherRefsRemained };

// Interfaces where refcounting is part of the public api should
// inherit this abstract interface. The implementation of these
// methods is usually provided by the RefCountedObject template class,
// applied as a leaf in the inheritance tree.
class RefCountInterface {
 public:
  virtual void AddRef() const = 0;
  virtual RefCountReleaseStatus Release() const = 0;

  // Non-public destructor, because Release() has exclusive responsibility for
  // destroying the object.
 protected:
  virtual ~RefCountInterface() {}
};

}  // namespace x2rtc

#endif  // X2RTC_BASE_REF_COUNT_H_
