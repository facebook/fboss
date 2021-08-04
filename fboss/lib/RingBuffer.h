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
template <typename T, size_t length>
class RingBuffer {
 public:
  using iterator = typename std::list<T>::iterator;

  void write(T val);
  const T last() const;
  bool empty() const;
  iterator begin();
  iterator end();
  size_t size();
  size_t maxSize();

 private:
  std::list<T> buf;
};

} // namespace facebook::fboss
