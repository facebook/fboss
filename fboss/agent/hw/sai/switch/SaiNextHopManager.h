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

#include "fboss/agent/hw/sai/api/NextHopApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/types.h"

#include "folly/IPAddress.h"

#include <memory>
#include <unordered_map>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;

using SaiIpNextHop = SaiObject<SaiIpNextHopTraits>;

class SaiNextHopManager {
 public:
  SaiNextHopManager(SaiManagerTable* managerTable, const SaiPlatform* platform);
  std::shared_ptr<SaiIpNextHop> addNextHop(
      RouterInterfaceSaiId routerInterfaceId,
      const folly::IPAddress& ip);

 private:
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
};

} // namespace facebook::fboss
