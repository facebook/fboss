// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/api/Srv6Api.h"

#include <boost/functional/hash.hpp>

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)

namespace std {

size_t hash<facebook::fboss::SaiMySidEntryTraits::MySidEntry>::operator()(
    const facebook::fboss::SaiMySidEntryTraits::MySidEntry& entry) const {
  size_t seed = 0;
  boost::hash_combine(seed, entry.switchId());
  boost::hash_combine(seed, entry.vrId());
  boost::hash_combine(seed, entry.locatorBlockLen());
  boost::hash_combine(seed, entry.locatorNodeLen());
  boost::hash_combine(seed, entry.functionLen());
  boost::hash_combine(seed, entry.argsLen());
  boost::hash_combine(seed, std::hash<folly::IPAddressV6>()(entry.sid()));
  return seed;
}

} // namespace std

namespace facebook::fboss {

std::string SaiMySidEntryTraits::MySidEntry::toString() const {
  return folly::to<std::string>(
      "MySidEntry(switch: ",
      switchId(),
      ", vr: ",
      vrId(),
      ", locatorBlockLen: ",
      locatorBlockLen(),
      ", locatorNodeLen: ",
      locatorNodeLen(),
      ", functionLen: ",
      functionLen(),
      ", argsLen: ",
      argsLen(),
      ", sid: ",
      sid(),
      ")");
}

} // namespace facebook::fboss

#endif
