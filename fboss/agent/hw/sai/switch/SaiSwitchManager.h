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
#include "fboss/agent/hw/sai/store/SaiStore.h"
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

class SaiSwitchManager {
 public:
  SaiSwitchManager(
      SaiManagerTable* managerTable,
      SaiPlatform* platform,
      BootType bootType,
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId);
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

  void setIngressAcl();
  void setIngressAcl(sai_object_id_t id);
  void resetIngressAcl();

  void setTamObject(std::vector<sai_object_id_t> tamObject);
  void resetTamObject();

  void setMacAgingSeconds(sai_uint32_t agingSeconds);
  sai_uint32_t getMacAgingSeconds() const;

  void setupCounterRefreshInterval();

  sai_object_id_t getDefaultVlanAdapterKey() const;

  PortSaiId getCpuPort() const;
  std::optional<PortSaiId> getCpuRecyclePort() const;

  void initCpuPort();
  void setCpuRecyclePort(PortSaiId portSaiId);

  void setPtpTcEnabled(bool ptpEnable);
  std::optional<bool> getPtpTcEnabled();

  bool isGlobalQoSMapSupported() const;
  bool isMplsQoSMapSupported() const;

  void updateStats();

  void configureCreditWatchdog(bool enable);
  void setSwitchIsolate(bool isolate);

 private:
  void programEcmpLoadBalancerParams(
      std::optional<sai_uint32_t> seed,
      std::optional<cfg::HashingAlgorithm> algo);
  void addOrUpdateEcmpLoadBalancer(const std::shared_ptr<LoadBalancer>& newLb);

  void programLagLoadBalancerParams(
      std::optional<sai_uint32_t> seed,
      std::optional<cfg::HashingAlgorithm> algo);
  void addOrUpdateLagLoadBalancer(const std::shared_ptr<LoadBalancer>& newLb);

  std::vector<sai_object_id_t> getUdfGroupIds(
      const std::shared_ptr<LoadBalancer>& newLb) const;

  template <typename HashAttrT>
  SaiHashTraits::CreateAttributes getProgrammedHashAttr();

  template <typename HashAttrT>
  void setLoadBalancer(
      const std::shared_ptr<SaiHash>& hash,
      SaiHashTraits::CreateAttributes& hashCreateAttrs);
  template <typename HashAttrT>
  void resetLoadBalancer();
  const std::vector<sai_stat_id_t>& supportedStats() const;

  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  std::unique_ptr<SaiSwitchObj> switch_;
  std::optional<PortSaiId> cpuPort_;
  std::optional<PortSaiId> cpuRecyclePort_;
  std::shared_ptr<SaiHash> ecmpV4Hash_;
  std::shared_ptr<SaiHash> ecmpV6Hash_;
  std::shared_ptr<SaiHash> lagV4Hash_;
  std::shared_ptr<SaiHash> lagV6Hash_;
  std::shared_ptr<SaiQosMap> globalDscpToTcQosMap_;
  std::shared_ptr<SaiQosMap> globalTcToQueueQosMap_;
  std::shared_ptr<SaiQosMap> globalExpToTcQosMap_;
  std::shared_ptr<SaiQosMap> globalTcToExpQosMap_;

  bool isMplsQosSupported_{false};
  // since this is an optional attribute in SAI
  std::optional<bool> isPtpTcEnabled_{std::nullopt};
};

} // namespace facebook::fboss
