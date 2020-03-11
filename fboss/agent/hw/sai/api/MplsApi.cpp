// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/MplsApi.h"

#include <boost/functional/hash.hpp>
#include "fboss/agent/Constants.h"

namespace std {
size_t hash<facebook::fboss::SaiInSegTraits::InSegEntry>::operator()(
    const facebook::fboss::SaiInSegTraits::InSegEntry& entry) const {
  size_t seed = 0;
  boost::hash_combine(seed, entry.switchId());
  boost::hash_combine(seed, entry.label());
  return seed;
}
} // namespace std

namespace facebook::fboss {

std::string SaiInSegTraits::InSegEntry::toString() const {
  return folly::to<std::string>(
      "InSegEntry(switch:", switchId(), ", label: ", label());
}
folly::dynamic SaiInSegTraits::InSegEntry::toFollyDynamic() const {
  folly::dynamic json = folly::dynamic::object;
  json[kSwitchId] = switchId();
  json[kLabel] = label();
  return json;
}

SaiInSegTraits::InSegEntry SaiInSegTraits::InSegEntry::fromFollyDynamic(
    const folly::dynamic& json) {
  sai_object_id_t switchId = json[kSwitchId].asInt();
  auto label = json[kLabel].asInt();
  return InSegEntry(switchId, label);
}
} // namespace facebook::fboss
