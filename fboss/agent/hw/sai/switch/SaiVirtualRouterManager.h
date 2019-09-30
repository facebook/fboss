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

#include "fboss/agent/hw/sai/api/VirtualRouterApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/types.h"

#include "folly/MacAddress.h"
#include "folly/container/F14Map.h"

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;

using SaiVirtualRouter = SaiObject<SaiVirtualRouterTraits>;
struct SaiVirtualRouterHandle {
  std::shared_ptr<SaiVirtualRouter> virtualRouter;
};

class SaiVirtualRouterManager {
 public:
  SaiVirtualRouterManager(
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);
  VirtualRouterSaiId addVirtualRouter(const RouterID& routerId);
  SaiVirtualRouterHandle* getVirtualRouterHandle(const RouterID& routerId);
  const SaiVirtualRouterHandle* getVirtualRouterHandle(
      const RouterID& routerId) const;

 private:
  SaiVirtualRouterHandle* getVirtualRouterHandleImpl(
      const RouterID& routerId) const;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  folly::F14FastMap<RouterID, std::unique_ptr<SaiVirtualRouterHandle>> handles_;
};

} // namespace facebook::fboss
