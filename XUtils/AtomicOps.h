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
#ifndef X2RTC_BASE_ATOMIC_OPS_H_
#define X2RTC_BASE_ATOMIC_OPS_H_

#if defined(_WIN32)
// clang-format off
// clang formating would change include order.

// Include winsock2.h before including <windows.h> to maintain consistency with
// win32.h. To include win32.h directly, it must be broken out into its own
// build target.
#include <winsock2.h>
#include <windows.h>
// clang-format on
#endif  // defined(_WIN32)

namespace x2rtc {
class AtomicOps {
 public:
#if defined(_WIN32)
  // Assumes sizeof(int) == sizeof(LONG), which it is on Win32 and Win64.
  static int Increment(volatile int* i) {
    return ::InterlockedIncrement(reinterpret_cast<volatile LONG*>(i));
  }
  static int Decrement(volatile int* i) {
    return ::InterlockedDecrement(reinterpret_cast<volatile LONG*>(i));
  }
  static int AcquireLoad(volatile const int* i) { return *i; }
  static void ReleaseStore(volatile int* i, int value) { *i = value; }
  static int CompareAndSwap(volatile int* i, int old_value, int new_value) {
    return ::InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(i),
                                        new_value, old_value);
  }
  // Pointer variants.
  template <typename T>
  static T* AcquireLoadPtr(T* volatile* ptr) {
    return *ptr;
  }
  template <typename T>
  static T* CompareAndSwapPtr(T* volatile* ptr, T* old_value, T* new_value) {
    return static_cast<T*>(::InterlockedCompareExchangePointer(
        reinterpret_cast<PVOID volatile*>(ptr), new_value, old_value));
  }
#else
  static int Increment(volatile int* i) { return __sync_add_and_fetch(i, 1); }
  static int Decrement(volatile int* i) { return __sync_sub_and_fetch(i, 1); }
  static int AcquireLoad(volatile const int* i) {
    return __atomic_load_n(i, __ATOMIC_ACQUIRE);
  }
  static void ReleaseStore(volatile int* i, int value) {
    __atomic_store_n(i, value, __ATOMIC_RELEASE);
  }
  static int CompareAndSwap(volatile int* i, int old_value, int new_value) {
    return __sync_val_compare_and_swap(i, old_value, new_value);
  }
  // Pointer variants.
  template <typename T>
  static T* AcquireLoadPtr(T* volatile* ptr) {
    return __atomic_load_n(ptr, __ATOMIC_ACQUIRE);
  }
  template <typename T>
  static T* CompareAndSwapPtr(T* volatile* ptr, T* old_value, T* new_value) {
    return __sync_val_compare_and_swap(ptr, old_value, new_value);
  }
#endif
};

}  // namespace x2rtc

#endif  // X2RTC_BASE_ATOMIC_OPS_H_
