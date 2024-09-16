/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <stddef.h>
#include <list>

namespace facebook::fboss {
template <typename T>
class RingBuffer {
 public:
  using iterator = typename std::list<T>::iterator;
  using const_iterator = typename std::list<T>::const_iterator;

  explicit RingBuffer<T>(size_t maxLength) : maxLength_(maxLength) {}

  void write(T val);
  const T last() const;
  bool empty() const;
  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  size_t size() const;
  size_t maxSize() const;

 private:
  std::list<T> buf;
  size_t maxLength_;
};

} // namespace facebook::fboss
