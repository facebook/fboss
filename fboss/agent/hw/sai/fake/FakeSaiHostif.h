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

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class FakeHostifTrap {
 public:
  FakeHostifTrap(
      sai_hostif_trap_type_t trapType,
      sai_packet_action_t packetAction,
      uint32_t priority,
      sai_object_id_t trapGroup)
      : trapType(trapType),
        packetAction(packetAction),
        priority(priority),
        trapGroup(trapGroup) {}
  sai_hostif_trap_type_t trapType;
  sai_int32_t packetAction;
  uint32_t priority;
  sai_object_id_t trapGroup;
  sai_object_id_t id;
};

class FakeHostifTrapGroup {
 public:
  explicit FakeHostifTrapGroup(uint32_t queueId, sai_object_id_t policer)
      : queueId(queueId), policerId(policer) {}
  uint32_t queueId;
  sai_object_id_t policerId;
  sai_object_id_t id;
};

using FakeHostifTrapManager = FakeManager<sai_object_id_t, FakeHostifTrap>;
using FakeHostifTrapGroupManager =
    FakeManager<sai_object_id_t, FakeHostifTrapGroup>;

void populate_hostif_api(sai_hostif_api_t** hostif_api);

} // namespace facebook::fboss
