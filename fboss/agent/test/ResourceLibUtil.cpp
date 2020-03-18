// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/state/RouteTypes.h"

namespace facebook::fboss::utility {

template <>
ResourceCursor<uint32_t>::ResourceCursor() : current_(0) {}

template <>
ResourceCursor<uint64_t>::ResourceCursor() : current_(0) {}

template <>
uint64_t ResourceCursor<uint64_t>::getNextId() {
  return ++current_;
}

template <>
uint32_t ResourceCursor<uint32_t>::getNextId() {
  return ++current_;
}

template <>
uint32_t ResourceCursor<uint32_t>::getId(uint32_t startId, uint32_t offset)
    const {
  return startId + offset;
}

template <>
uint64_t ResourceCursor<uint64_t>::getId(uint64_t startId, uint32_t offset)
    const {
  return startId + offset;
}

template <>
ResourceCursor<IdV6>::ResourceCursor() : current_{0, 0} {}

template <>
IdV6 ResourceCursor<IdV6>::getNextId() {
  if (current_.second < static_cast<uint64_t>(-1)) {
    ++current_.second;
  } else {
    ++current_.first;
  }
  return current_;
}

template <>
IdV6 ResourceCursor<IdV6>::getId(IdV6 startId, uint32_t offset) const {
  if ((startId.second + offset) < static_cast<uint64_t>(-1)) {
    /* if adding an offset to second doesn't exceed the uint64_t limit, then
     * simply add it */
    startId.second += offset;
  } else {
    /* if adding an offset to second exceeds the uint64_t limit, then
     * second absorbs (max - second) units, and becomes max
     * first absors remaining units which are (offset - units absored by second)
     */
    startId.first += (offset - (static_cast<uint64_t>(-1) - startId.second));
    startId.second = static_cast<uint64_t>(-1);
  }
  return startId;
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

template class IPAddressGenerator<folly::IPAddressV4>;
template class IPAddressGenerator<folly::IPAddressV6>;
template class RouteGenerator<folly::IPAddressV4>;
template class RouteGenerator<folly::IPAddressV6>;

} // namespace facebook::fboss::utility
