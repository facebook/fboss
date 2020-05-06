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
#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {

class LookupClassRouteUpdater : public AutoRegisterStateObserver {
 public:
  explicit LookupClassRouteUpdater(SwSwitch* sw)
      : AutoRegisterStateObserver(sw, "LookupClassRouteUpdater") {}
  ~LookupClassRouteUpdater() override {}

  void stateUpdated(const StateDelta& stateDelta) override;
};

} // namespace facebook::fboss
