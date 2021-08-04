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

#include "fboss/agent/FbossError.h"
#include "fboss/lib/RingBuffer.h"

namespace facebook::fboss {

template <typename T, size_t length>
void RingBuffer<T, length>::write(T val) {
  if (buf.size() == length) {
    buf.pop_front();
  }
  buf.push_back(val);
}

template <typename T, size_t length>
const T RingBuffer<T, length>::last() const {
  if (buf.empty()) {
    throw FbossError("Attempted to read from empty RingBuffer");
  }
  return buf.back();
}

template <typename T, size_t length>
bool RingBuffer<T, length>::empty() const {
  return buf.empty();
}

template <typename T, size_t length>
typename RingBuffer<T, length>::iterator RingBuffer<T, length>::begin() {
  return buf.begin();
}

template <typename T, size_t length>
typename RingBuffer<T, length>::iterator RingBuffer<T, length>::end() {
  return buf.end();
}

template <typename T, size_t length>
size_t RingBuffer<T, length>::size() {
  return buf.size();
}

template <typename T, size_t length>
size_t RingBuffer<T, length>::maxSize() {
  return length;
}

} // namespace facebook::fboss
