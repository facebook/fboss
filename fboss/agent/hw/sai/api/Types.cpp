// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/Types.h"

#include <boost/functional/hash.hpp>

namespace std {

size_t hash<facebook::fboss::PortDescriptorSaiId>::operator()(
    const facebook::fboss::PortDescriptorSaiId& key) const {
  size_t seed = 0;
  boost::hash_combine(seed, key.intID());
  return seed;
}

} // namespace std

namespace facebook::fboss {

SaiTimeSpec fromSaiTimeSpec(sai_timespec_t sai_timespec) {
  return SaiTimeSpec{
      .tv_sec = static_cast<std::time_t>(sai_timespec.tv_sec),
      .tv_nsec = sai_timespec.tv_nsec};
}

void toSaiTimeSpec(const SaiTimeSpec& spec, sai_timespec_t& sai_timespec) {
  sai_timespec.tv_sec = static_cast<uint64_t>(spec.tv_sec);
  sai_timespec.tv_nsec = static_cast<uint32_t>(spec.tv_nsec);
}

} // namespace facebook::fboss
