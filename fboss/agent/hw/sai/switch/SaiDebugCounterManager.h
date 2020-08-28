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

#include "fboss/agent/hw/sai/api/DebugCounterApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"

#include "fboss/lib/RefMap.h"

namespace facebook::fboss {

class SaiManagerTable;
class StateDelta;

using SaiDebugCounter = SaiObject<SaiDebugCounterTraits>;

class SaiDebugCounterManager {
 public:
  explicit SaiDebugCounterManager(SaiManagerTable* managerTable)
      : managerTable_(managerTable) {}

  void setupDebugCounters();
  const SaiDebugCounter* getPortL3BlackHoleCounter() const {
    return portL3BlackHoleCounter_.get();
  }

 private:
  std::unique_ptr<SaiDebugCounter> portL3BlackHoleCounter_;
  SaiManagerTable* managerTable_;
};

} // namespace facebook::fboss
