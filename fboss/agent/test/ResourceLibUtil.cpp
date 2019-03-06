// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/test/ResourceLibUtil.h"

namespace facebook {
namespace fboss {
namespace utility {

template <>
ResourceCursor<uint32_t>::ResourceCursor() : current_(0) {}

template <>
uint32_t ResourceCursor<uint32_t>::getNext() {
  return ++current_;
}

template <>
void ResourceCursor<uint32_t>::resetCursor(uint32_t current) {
  current_ = current;
}

template <>
uint32_t ResourceCursor<uint32_t>::getCursorPosition() const {
  return current_;
}
} // namespace utility
} // namespace fboss
} // namespace facebook
