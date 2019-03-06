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

template <>
folly::IPAddressV4 IPAddressGenerator<folly::IPAddressV4>::getIP(
    uint32_t id) const {
  return folly::IPAddressV4::fromLongHBO(id);
}

template <>
std::vector<folly::IPAddressV4>
IPAddressGenerator<folly::IPAddressV4>::getNextN(uint32_t n) {
  std::vector<folly::IPAddressV4> resources;
  for (auto i = 0; i < n; i++) {
    resources.emplace_back(getNext());
  }
  return resources;
}

template <>
std::vector<folly::IPAddressV4> IPAddressGenerator<folly::IPAddressV4>::getN(
    uint32_t startId,
    uint32_t n) const {
  std::vector<folly::IPAddressV4> resources;
  for (auto i = 0; i < n; i++) {
    resources.emplace_back(get(startId + i));
  }
  return resources;
}

template class IPAddressGenerator<folly::IPAddressV4>;
} // namespace utility
} // namespace fboss
} // namespace facebook
