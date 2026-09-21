/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/HashApi.h"

#include <boost/functional/hash.hpp>

#include <functional>

namespace std {
size_t hash<facebook::fboss::SaiHashTraits::AdapterHostKey>::operator()(
    const facebook::fboss::SaiHashTraits::AdapterHostKey& key) const {
  size_t seed = 0;
  if (std::get<0>(key)) {
    for (auto field : std::get<0>(key).value().value()) {
      boost::hash_combine(seed, field);
    }
  }
  if (std::get<1>(key)) {
    for (auto udf : std::get<1>(key).value().value()) {
      boost::hash_combine(seed, udf);
    }
  }
  return seed;
}
} // namespace std
