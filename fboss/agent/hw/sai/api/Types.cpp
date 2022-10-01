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
