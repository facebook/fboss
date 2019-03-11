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

#include <memory>

namespace facebook {
namespace fboss {

class SaiApiTable;
class SaiBridgeManager;
class SaiNeighborManager;
class SaiNextHopManager;
class SaiNextHopGroupManager;
class SaiPortManager;
class SaiRouterInterfaceManager;
class SaiVirtualRouterManager;
class SaiVlanManager;

class SaiManagerTable {
 public:
  explicit SaiManagerTable(SaiApiTable* apiTable);
  ~SaiManagerTable();

  SaiBridgeManager& bridgeManager();
  const SaiBridgeManager& bridgeManager() const;

  SaiNeighborManager& neighborManager();
  const SaiNeighborManager& neighborManager() const;

  SaiNextHopManager& nextHopManager();
  const SaiNextHopManager& nextHopManager() const;

  SaiNextHopGroupManager& nextHopGroupManager();
  const SaiNextHopGroupManager& nextHopGroupManager() const;

  SaiPortManager& portManager();
  const SaiPortManager& portManager() const;

  SaiRouterInterfaceManager& routerInterfaceManager();
  const SaiRouterInterfaceManager& routerInterfaceManager() const;

  SaiVirtualRouterManager& virtualRouterManager();
  const SaiVirtualRouterManager& virtualRouterManager() const;

  SaiVlanManager& vlanManager();
  const SaiVlanManager& vlanManager() const;

 private:
  std::unique_ptr<SaiBridgeManager> bridgeManager_;
  std::unique_ptr<SaiNeighborManager> neighborManager_;
  std::unique_ptr<SaiNextHopManager> nextHopManager_;
  std::unique_ptr<SaiNextHopGroupManager> nextHopGroupManager_;
  std::unique_ptr<SaiPortManager> portManager_;
  std::unique_ptr<SaiRouterInterfaceManager> routerInterfaceManager_;
  std::unique_ptr<SaiVirtualRouterManager> virtualRouterManager_;
  std::unique_ptr<SaiVlanManager> vlanManager_;
  SaiApiTable* apiTable_;
};

} // namespace fboss
} // namespace facebook
