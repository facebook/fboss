// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/state/RouteTypes.h"

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
ResourceCursor<IdV6>::ResourceCursor() : current_{0,0} {}

template <>
IdV6 ResourceCursor<IdV6>::getNext() {
  if (current_.second < static_cast<uint64_t>(-1)) {
    ++current_.second;
  } else {
    ++current_.first;
  }
  return current_;
}

template <>
void ResourceCursor<IdV6>::resetCursor(IdV6 current) {
  current_ = current;
}

template <>
IdV6 ResourceCursor<IdV6>::getCursorPosition() const {
  return current_;
}

template <>
folly::IPAddressV4 IPAddressGenerator<folly::IPAddressV4>::getIP(
    uint32_t id) const {
  return folly::IPAddressV4::fromLongHBO(id);
}

template <>
folly::IPAddressV6 IPAddressGenerator<folly::IPAddressV6>::getIP(
    IdV6 id) const {
  /*
   * For given id, generate corresponding IPv6 address
   *    1->::1, 2->::2, 3->::3 and so on
   *
   * Transform give 128 byte id, into byte range of 16 bytes; which then is used
   * to generate IPv6 address
   *
   */

  std::array<uint8_t, 16> buffer;
  auto* first = reinterpret_cast<uint8_t*>(&id.first);
  auto* second = reinterpret_cast<uint8_t*>(&id.second);
  for (int i = 15; i >= 0; i--) {
    /* pick first or second part of pair based on i */
    uint8_t* b8 = (i / 8) ? second : first;
    /* b8 is 8 byte long, pick right byte out of b8 and place it in buffer */
    buffer[i] = b8[7 - i % 8];
  }
  return folly::IPAddressV6::fromBinary(
      folly::ByteRange(buffer.begin(), buffer.end()));
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
std::vector<folly::IPAddressV6>
IPAddressGenerator<folly::IPAddressV6>::getNextN(uint32_t n) {
  std::vector<folly::IPAddressV6> resources;
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

template <>
std::vector<folly::IPAddressV6> IPAddressGenerator<folly::IPAddressV6>::getN(
    IdV6 startId,
    uint32_t n) const {
  std::vector<folly::IPAddressV6> resources;
  for (auto i = 0; i < n; i++) {
    IdV6 id = startId;
    if (startId.second < static_cast<uint64_t>(-1)) {
      id.second += i;
    } else {
      id.first += i;
    }
    resources.emplace_back(get(id));
  }
  return resources;
}

template class IPAddressGenerator<folly::IPAddressV4>;
template class IPAddressGenerator<folly::IPAddressV6>;

} // namespace utility
} // namespace fboss
} // namespace facebook
