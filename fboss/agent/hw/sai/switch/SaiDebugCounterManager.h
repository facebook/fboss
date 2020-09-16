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

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class StateDelta;

using SaiDebugCounter = SaiObject<SaiDebugCounterTraits>;

class SaiDebugCounterManager {
 public:
  explicit SaiDebugCounterManager(SaiManagerTable* managerTable)
      : managerTable_(managerTable) {}

  void setupDebugCounters();
  sai_stat_id_t getPortL3BlackHoleCounterStatId() const {
    CHECK(portL3BlackHoleCounter_);
    return portL3BlackHoleCounterStatId_;
  }

 private:
  std::shared_ptr<SaiDebugCounter> portL3BlackHoleCounter_;
  sai_stat_id_t portL3BlackHoleCounterStatId_{0};
  SaiManagerTable* managerTable_;
};

} // namespace facebook::fboss
