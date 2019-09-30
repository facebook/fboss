/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "FakeManager.h"

#include <unordered_map>
#include <vector>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class FakePort {
 public:
  FakePort(std::vector<uint32_t> lanes, sai_uint32_t speed)
      : lanes(lanes), speed(speed) {}
  sai_object_id_t id;
  bool adminState{false};
  std::vector<uint32_t> lanes;
  uint32_t speed{0};
  sai_port_fec_mode_t fecMode{SAI_PORT_FEC_MODE_NONE};
  sai_port_internal_loopback_mode_t internalLoopbackMode{
      SAI_PORT_INTERNAL_LOOPBACK_MODE_NONE};
  sai_port_flow_control_mode_t globalFlowControlMode{
      SAI_PORT_FLOW_CONTROL_MODE_DISABLE};
  sai_port_media_type_t mediaType{SAI_PORT_MEDIA_TYPE_NOT_PRESENT};
  sai_vlan_id_t vlanId{0};
};

using FakePortManager = FakeManager<sai_object_id_t, FakePort>;

void populate_port_api(sai_port_api_t** port_api);

} // namespace facebook::fboss
