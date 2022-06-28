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

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <optional>

namespace facebook::fboss {

struct SystemPortFields
    : public BetterThriftyFields<SystemPortFields, state::SystemPortFields> {
  explicit SystemPortFields(SystemPortID id) {
    *data.portId() = id;
  }

  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}

  state::SystemPortFields toThrift() const {
    return data;
  }
  static SystemPortFields fromThrift(
      const state::SystemPortFields& systemPortThrift);
};

class SystemPort : public ThriftyBaseT<
                       state::SystemPortFields,
                       SystemPort,
                       SystemPortFields> {
 private:
  // Inherit the constructors required for clone()
  using ThriftyBaseT<state::SystemPortFields, SystemPort, SystemPortFields>::
      ThriftyBaseT;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
