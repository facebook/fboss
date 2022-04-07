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

#include "fboss/agent/hw/sai/api/CounterApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiObjectWithCounters.h"

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class StateDelta;
class SaiStore;
class SaiPlatform;

using SaiCounter = SaiObjectWithCounters<SaiCounterTraits>;

class SaiCounterManager {
 public:
  explicit SaiCounterManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      SaiPlatform* platform)
      : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

 private:
  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  SaiPlatform* platform_;
};

} // namespace facebook::fboss
