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
#include "fboss/agent/hw/sai/api/fake/saifakeextensions.h"
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
  std::vector<sai_object_id_t> queueIdList;
  std::vector<uint32_t> preemphasis;
  sai_uint32_t mtu{1514};
  sai_object_id_t qosDscpToTcMap{SAI_NULL_OBJECT_ID};
  sai_object_id_t qosTcToQueueMap{SAI_NULL_OBJECT_ID};
  bool disableTtlDecrement{false};
  sai_port_interface_type_t interface_type{SAI_PORT_INTERFACE_TYPE_NONE};
};

struct FakePortSerdes {
  explicit FakePortSerdes(sai_object_id_t _port) : port(_port) {}
  sai_object_id_t id;
  sai_object_id_t port;
  std::vector<uint32_t> iDriver;
  std::vector<uint32_t> txFirPre1;
  std::vector<uint32_t> txFirPre2;
  std::vector<uint32_t> txFirMain;
  std::vector<uint32_t> txFirPost1;
  std::vector<uint32_t> txFirPost2;
  std::vector<uint32_t> txFirPost3;
  std::vector<int32_t> rxCtlCode;
  std::vector<int32_t> rxDspMode;
  std::vector<int32_t> rxAfeTrim;
  std::vector<int32_t> rxCouplingByPass;
};

using FakePortManager = FakeManager<sai_object_id_t, FakePort>;
using FakePortSerdesManager = FakeManager<sai_object_id_t, FakePortSerdes>;

void populate_port_api(sai_port_api_t** port_api);

} // namespace facebook::fboss
