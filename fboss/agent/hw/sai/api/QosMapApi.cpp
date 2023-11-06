/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/QosMapApi.h"
#include <boost/functional/hash.hpp>

namespace std {
size_t hash<facebook::fboss::SaiQosMapTraits::AdapterHostKey>::operator()(
    const facebook::fboss::SaiQosMapTraits::AdapterHostKey& key) const {
  size_t seed = 0;
  boost::hash_combine(seed, std::get<0>(key).value());
  for (auto mapToVal : std::get<1>(key).value()) {
    for (auto& param : {mapToVal.key, mapToVal.value}) {
      boost::hash_combine(seed, param.tc);
      boost::hash_combine(seed, param.dscp);
      boost::hash_combine(seed, param.prio);
      boost::hash_combine(seed, param.pg);
      boost::hash_combine(seed, param.queue_index);
      boost::hash_combine(seed, param.color);
      boost::hash_combine(seed, param.mpls_exp);
    }
  }
  return seed;
}
} // namespace std

bool operator==(
    const sai_qos_map_params_t& lhs,
    const sai_qos_map_params_t& rhs) {
  return lhs.tc == rhs.tc && lhs.dscp == rhs.dscp && lhs.dot1p == rhs.dot1p &&
      lhs.prio == rhs.prio && lhs.pg == rhs.pg &&
      lhs.queue_index == rhs.queue_index && lhs.color == rhs.color;
}

bool operator==(const sai_qos_map_t& lhs, const sai_qos_map_t& rhs) {
  return lhs.key == rhs.key && lhs.value == rhs.value;
}

bool operator!=(
    const sai_qos_map_params_t& lhs,
    const sai_qos_map_params_t& rhs) {
  return !(lhs == rhs);
}

bool operator!=(const sai_qos_map_t& lhs, const sai_qos_map_t& rhs) {
  return !(lhs == rhs);
}
