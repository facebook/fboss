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

#include "fboss/agent/hw/sai/fake/FakeManager.h"

#include <unordered_map>
#include <vector>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

struct FakeSystemPort {
  FakeSystemPort(
      const sai_system_port_config_t& config,
      sai_object_id_t switchId,
      bool adminState,
      sai_object_id_t qosToTcMapId)
      : config(config),
        switchId(switchId),
        adminState(adminState),
        qosToTcMapId(qosToTcMapId) {}
  sai_object_id_t id;
  sai_system_port_config_t config;
  sai_object_id_t switchId;
  bool adminState{false};
  sai_object_id_t qosToTcMapId;
  std::vector<sai_object_id_t> queueIdList;
};

using FakeSystemPortManager = FakeManager<sai_object_id_t, FakeSystemPort>;

void populate_system_port_api(sai_system_port_api_t** system_port_api);
}; // namespace facebook::fboss
