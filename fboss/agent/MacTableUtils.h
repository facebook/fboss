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

#include "fboss/agent/L2Entry.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook {
namespace fboss {

class SwitchState;
class Vlan;

class MacTableUtils {
 public:
  static std::shared_ptr<SwitchState> updateMacTable(
      const std::shared_ptr<SwitchState>& state,
      L2Entry l2Entry,
      L2EntryUpdateType l2EntryUpdateType);
};

} // namespace fboss
} // namespace facebook
