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

#include "fboss/agent/StateObserver.h"
#include "fboss/agent/types.h"

#include <memory>

namespace facebook::fboss {
class SwitchState;
class StateDelta;

class StaticL2ForNeighborObserver : public AutoRegisterStateObserver {
 public:
  explicit StaticL2ForNeighborObserver(SwSwitch* sw)
      : AutoRegisterStateObserver(sw, "StaticL2ForNeighborObserver"), sw_(sw) {}
  ~StaticL2ForNeighborObserver() override {}

  void stateUpdated(const StateDelta& stateDelta) override;

 private:
  SwSwitch* sw_;
};
} // namespace facebook::fboss
