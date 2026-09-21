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

class FakeBridgePort {
 public:
  explicit FakeBridgePort(
      int32_t type,
      sai_object_id_t portId,
      bool adminState,
      int32_t learningMode)
      : type(type),
        portId(portId),
        adminState(adminState),
        learningMode(learningMode) {}
  sai_object_id_t id;
  int32_t type;
  sai_object_id_t portId;
  bool adminState;
  int32_t learningMode;
};

class FakeBridge {
 public:
  explicit FakeBridge(sai_bridge_type_t type) : type(type) {}
  // Default bridge type is 1Q.
  FakeBridge() : type(SAI_BRIDGE_TYPE_1Q) {}
  sai_object_id_t id;
  sai_bridge_type_t type;
  FakeManager<sai_object_id_t, FakeBridgePort>& fm() {
    return fm_;
  }
  const FakeManager<sai_object_id_t, FakeBridgePort>& fm() const {
    return fm_;
  }

 private:
  FakeManager<sai_object_id_t, FakeBridgePort> fm_;
};

using FakeBridgeManager = FakeManagerWithMembers<FakeBridge, FakeBridgePort>;

void populate_bridge_api(sai_bridge_api_t** bridge_api);

} // namespace facebook::fboss
