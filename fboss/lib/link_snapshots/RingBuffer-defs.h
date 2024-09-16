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
#include "fboss/lib/link_snapshots/RingBuffer.h"

namespace facebook::fboss {

template <typename T>
void RingBuffer<T>::write(T val) {
  if (buf.size() == maxLength_) {
    buf.pop_front();
  }
  buf.push_back(val);
}

template <typename T>
const T RingBuffer<T>::last() const {
  if (buf.empty()) {
    throw FbossError("Attempted to read from empty RingBuffer");
  }
  return buf.back();
}

template <typename T>
bool RingBuffer<T>::empty() const {
  return buf.empty();
}

template <typename T>
typename RingBuffer<T>::iterator RingBuffer<T>::begin() {
  return buf.begin();
}

template <typename T>
typename RingBuffer<T>::iterator RingBuffer<T>::end() {
  return buf.end();
}

template <typename T>
typename RingBuffer<T>::const_iterator RingBuffer<T>::begin() const {
  return buf.begin();
}

template <typename T>
typename RingBuffer<T>::const_iterator RingBuffer<T>::end() const {
  return buf.end();
}

template <typename T>
size_t RingBuffer<T>::size() const {
  return buf.size();
}

template <typename T>
size_t RingBuffer<T>::maxSize() const {
  return maxLength_;
}

} // namespace facebook::fboss
