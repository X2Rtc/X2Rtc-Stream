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
#ifndef X2RTC_BASE_REF_COUNTER_H_
#define X2RTC_BASE_REF_COUNTER_H_

#include "AtomicOps.h"
#include "RefCount.h"

namespace x2rtc {

class RefCounter {
 public:
  explicit RefCounter(int ref_count) : ref_count_(ref_count) {}
  RefCounter() = delete;

  void IncRef() { x2rtc::AtomicOps::Increment(&ref_count_); }

  // Returns kDroppedLastRef if this call dropped the last reference; the caller
  // should therefore free the resource protected by the reference counter.
  // Otherwise, returns kOtherRefsRemained (note that in case of multithreading,
  // some other caller may have dropped the last reference by the time this call
  // returns; all we know is that we didn't do it).
  x2rtc::RefCountReleaseStatus DecRef() {
    return (x2rtc::AtomicOps::Decrement(&ref_count_) == 0)
               ? x2rtc::RefCountReleaseStatus::kDroppedLastRef
               : x2rtc::RefCountReleaseStatus::kOtherRefsRemained;
  }

  // Return whether the reference count is one. If the reference count is used
  // in the conventional way, a reference count of 1 implies that the current
  // thread owns the reference and no other thread shares it. This call performs
  // the test for a reference count of one, and performs the memory barrier
  // needed for the owning thread to act on the resource protected by the
  // reference counter, knowing that it has exclusive access.
  bool HasOneRef() const {
    return x2rtc::AtomicOps::AcquireLoad(&ref_count_) == 1;
  }

 private:
  volatile int ref_count_;
};

}  // namespace x2rtc

#endif  // X2RTC_BASE_REF_COUNTER_H_
