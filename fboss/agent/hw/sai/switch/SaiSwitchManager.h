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
#include "fboss/agent/hw/sai/switch/SaiHashManager.h"
#include "fboss/agent/types.h"

#include "folly/MacAddress.h"

#include <memory>
#include <unordered_map>

namespace facebook::fboss {

class LoadBalancer;
class SaiManagerTable;
class SaiPlatform;
class StateDelta;

class SaiSwitchInstance {
 public:
  explicit SaiSwitchInstance(
      const SaiSwitchTraits::CreateAttributes& attributes);
  ~SaiSwitchInstance();
  SaiSwitchInstance(const SaiSwitchInstance& other) = delete;
  SaiSwitchInstance(SaiSwitchInstance&& other) = delete;
  SaiSwitchInstance& operator=(const SaiSwitchInstance& other) = delete;
  SaiSwitchInstance& operator=(SaiSwitchInstance&& other) = delete;
  bool operator==(const SaiSwitchInstance& other) const;
  bool operator!=(const SaiSwitchInstance& other) const;

  const SaiSwitchTraits::CreateAttributes attributes() const {
    return attributes_;
  }
  SwitchSaiId id() const {
    return id_;
  }

 private:
  SaiSwitchTraits::CreateAttributes attributes_;
  SwitchSaiId id_;
};

class SaiSwitchManager {
 public:
  SaiSwitchManager(SaiManagerTable* managerTable, SaiPlatform* platform);
  const SaiSwitchInstance* getSwitch() const;
  SaiSwitchInstance* getSwitch();
  SwitchSaiId getSwitchSaiId() const;

  void processLoadBalancerDelta(const StateDelta& delta);

 private:
  void programLoadBalancerParams(
      cfg::LoadBalancerID id,
      std::optional<sai_uint32_t> seed,
      std::optional<cfg::HashingAlgorithm> algo);
  void addOrUpdateLoadBalancer(const std::shared_ptr<LoadBalancer>& newLb);
  void removeLoadBalancer(const std::shared_ptr<LoadBalancer>& oldLb);

  SaiSwitchInstance* getSwitchImpl() const;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  std::unique_ptr<SaiSwitchInstance> switch_;
  std::shared_ptr<SaiHash> ecmpV4Hash_;
  std::shared_ptr<SaiHash> ecmpV6Hash_;
};

} // namespace facebook::fboss
