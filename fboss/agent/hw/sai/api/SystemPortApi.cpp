/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/SystemPortApi.h"

bool operator==(
    const sai_system_port_config_t& lhs,
    const sai_system_port_config_t& rhs) {
  return std::tie(
             lhs.port_id,
             lhs.attached_switch_id,
             lhs.attached_core_index,
             lhs.attached_core_port_index,
             lhs.speed,
             lhs.num_voq) ==
      std::tie(
             rhs.port_id,
             rhs.attached_switch_id,
             rhs.attached_core_index,
             rhs.attached_core_port_index,
             rhs.speed,
             rhs.num_voq);
}

bool operator!=(
    const sai_system_port_config_t& lhs,
    const sai_system_port_config_t& rhs) {
  return !(rhs == lhs);
}
