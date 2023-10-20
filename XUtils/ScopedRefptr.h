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
#ifndef X2RTC_SCOPED_REFPTR_H_
#define X2RTC_SCOPED_REFPTR_H_

#include <memory>
#include <utility>

namespace x2rtc {

template <class T>
class scoped_refptr {
 public:
  typedef T element_type;

  scoped_refptr() : ptr_(nullptr) {}

  scoped_refptr(T* p) : ptr_(p) {  // NOLINT(runtime/explicit)
    if (ptr_)
      ptr_->AddRef();
  }

  scoped_refptr(const scoped_refptr<T>& r) : ptr_(r.ptr_) {
    if (ptr_)
      ptr_->AddRef();
  }

  template <typename U>
  scoped_refptr(const scoped_refptr<U>& r) : ptr_(r.get()) {
    if (ptr_)
      ptr_->AddRef();
  }

  // Move constructors.
  scoped_refptr(scoped_refptr<T>&& r) noexcept : ptr_(r.release()) {}

  template <typename U>
  scoped_refptr(scoped_refptr<U>&& r) noexcept : ptr_(r.release()) {}

  ~scoped_refptr() {
    if (ptr_)
      ptr_->Release();
  }

  T* get() const { return ptr_; }
  operator T*() const { return ptr_; }
  T& operator*() const { return *ptr_; }
  T* operator->() const { return ptr_; }

  // Returns the (possibly null) raw pointer, and makes the scoped_refptr hold a
  // null pointer, all without touching the reference count of the underlying
  // pointed-to object. The object is still reference counted, and the caller of
  // release() is now the proud owner of one reference, so it is responsible for
  // calling Release() once on the object when no longer using it.
  T* release() {
    T* retVal = ptr_;
    ptr_ = nullptr;
    return retVal;
  }

  scoped_refptr<T>& operator=(T* p) {
    // AddRef first so that self assignment should work
    if (p)
      p->AddRef();
    if (ptr_)
      ptr_->Release();
    ptr_ = p;
    return *this;
  }

  scoped_refptr<T>& operator=(const scoped_refptr<T>& r) {
    return *this = r.ptr_;
  }

  template <typename U>
  scoped_refptr<T>& operator=(const scoped_refptr<U>& r) {
    return *this = r.get();
  }

  scoped_refptr<T>& operator=(scoped_refptr<T>&& r) noexcept {
    scoped_refptr<T>(std::move(r)).swap(*this);
    return *this;
  }

  template <typename U>
  scoped_refptr<T>& operator=(scoped_refptr<U>&& r) noexcept {
    scoped_refptr<T>(std::move(r)).swap(*this);
    return *this;
  }

  void swap(T** pp) noexcept {
    T* p = ptr_;
    ptr_ = *pp;
    *pp = p;
  }

  void swap(scoped_refptr<T>& r) noexcept { swap(&r.ptr_); }

 protected:
  T* ptr_;
};

}  // namespace x2rtc

#endif  // X2RTC_SCOPED_REFPTR_H_
