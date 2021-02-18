// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/PortDescriptorTemplate.h"

#include <boost/functional/hash.hpp>

namespace std {

size_t hash<facebook::fboss::BasePortDescriptor>::operator()(
    const facebook::fboss::BasePortDescriptor& key) const {
  size_t seed = 0;
  uint64_t keyValue = key.isPhysicalPort() ? key.phyPortID() : key.aggPortID();
  boost::hash_combine(seed, keyValue);
  return seed;
}
} // namespace std
