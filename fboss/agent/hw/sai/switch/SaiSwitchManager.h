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

#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/VirtualRouterApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/switch/SaiHashManager.h"
#include "fboss/agent/hw/sai/switch/SaiQosMapManager.h"
#include "fboss/agent/types.h"

#include <folly/MacAddress.h>

#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace folly {
struct dynamic;
}
namespace facebook::fboss {

class LoadBalancer;
class SaiManagerTable;
class SaiPlatform;
class StateDelta;

using SaiSwitchObj = SaiObject<SaiSwitchTraits>;

class SaiSwitchManager {
 public:
  SaiSwitchManager(
      SaiManagerTable* managerTable,
      SaiPlatform* platform,
      BootType bootType);
  SwitchSaiId getSwitchSaiId() const;

  void setQosPolicy();
  void clearQosPolicy();
  void addOrUpdateLoadBalancer(const std::shared_ptr<LoadBalancer>& newLb);
  void changeLoadBalancer(
      const std::shared_ptr<LoadBalancer>& oldLb,
      const std::shared_ptr<LoadBalancer>& newLb);
  void removeLoadBalancer(const std::shared_ptr<LoadBalancer>& oldLb);

  void resetHashes();
  void resetQosMaps();
  void gracefulExit();

  void setMacAgingSeconds(sai_uint32_t agingSeconds);
  sai_uint32_t getMacAgingSeconds() const;

 private:
  void programLoadBalancerParams(
      cfg::LoadBalancerID id,
      std::optional<sai_uint32_t> seed,
      std::optional<cfg::HashingAlgorithm> algo);

  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  std::unique_ptr<SaiSwitchObj> switch_;
  std::shared_ptr<SaiHash> ecmpV4Hash_;
  std::shared_ptr<SaiHash> ecmpV6Hash_;
  std::shared_ptr<SaiQosMap> globalDscpToTcQosMap_;
  std::shared_ptr<SaiQosMap> globalTcToQueueQosMap_;
};

} // namespace facebook::fboss
