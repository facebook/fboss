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

#include "fboss/agent/hw/sai/api/UdfApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;

using SaiUdf = SaiObject<SaiUdfTraits>;
using SaiUdfGroup = SaiObject<SaiUdfGroupTraits>;
using SaiUdfMatch = SaiObject<SaiUdfMatchTraits>;

class SaiUdfManager {
 public:
  SaiUdfManager(SaiManagerTable* managerTable, const SaiPlatform* platform)
      : managerTable_(managerTable), platform_(platform) {}

 private:
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
};

} // namespace facebook::fboss
