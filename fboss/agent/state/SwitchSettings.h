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

#include <folly/MacAddress.h>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/FlowletSwitchingConfig.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/QcmConfig.h"
#include "fboss/agent/state/QosPolicyMap.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/state/UdfConfig.h"

namespace facebook::fboss {

class SwitchState;

using SwitchIdToSwitchInfo = std::map<int64_t, cfg::SwitchInfo>;

USE_THRIFT_COW(SwitchSettings)
RESOLVE_STRUCT_MEMBER(SwitchSettings, switch_state_tags::qcmCfg, QcmCfg)
RESOLVE_STRUCT_MEMBER(
    SwitchSettings,
    switch_state_tags::defaultDataPlaneQosPolicy,
    QosPolicy)
RESOLVE_STRUCT_MEMBER(SwitchSettings, switch_state_tags::udfConfig, UdfConfig)
RESOLVE_STRUCT_MEMBER(
    SwitchSettings,
    switch_state_tags::flowletSwitchingConfig,
    FlowletSwitchingConfig)

/*
 * SwitchSettings stores state about path settings of traffic to userver CPU
 * on the switch.
 */
class SwitchSettings
    : public ThriftStructNode<SwitchSettings, state::SwitchSettingsFields> {
 public:
  using BaseT = ThriftStructNode<SwitchSettings, state::SwitchSettingsFields>;
  using BaseT::modify;

  cfg::L2LearningMode getL2LearningMode() const {
    return cref<switch_state_tags::l2LearningMode>()->toThrift();
  }

  void setL2LearningMode(cfg::L2LearningMode l2LearningMode) {
    set<switch_state_tags::l2LearningMode>(l2LearningMode);
  }

  bool isQcmEnable() const {
    return cref<switch_state_tags::qcmEnable>()->toThrift();
  }

  void setQcmEnable(const bool enable) {
    set<switch_state_tags::qcmEnable>(enable);
  }

  bool isPtpTcEnable() const {
    return cref<switch_state_tags::ptpTcEnable>()->toThrift();
  }

  void setPtpTcEnable(const bool enable) {
    set<switch_state_tags::ptpTcEnable>(enable);
  }

  uint32_t getL2AgeTimerSeconds() const {
    return cref<switch_state_tags::l2AgeTimerSeconds>()->toThrift();
  }

  void setL2AgeTimerSeconds(uint32_t val) {
    set<switch_state_tags::l2AgeTimerSeconds>(val);
  }

  uint32_t getMaxRouteCounterIDs() const {
    return cref<switch_state_tags::maxRouteCounterIDs>()->toThrift();
  }

  void setMaxRouteCounterIDs(uint32_t numCounterIDs) {
    set<switch_state_tags::maxRouteCounterIDs>(numCounterIDs);
  }

  auto getBlockNeighbors() const {
    return safe_cref<switch_state_tags::blockNeighbors>();
  }

  // THRIFT_COPY
  std::vector<std::pair<VlanID, folly::IPAddress>>
  getBlockNeighbors_DEPRECATED() const {
    std::vector<std::pair<VlanID, folly::IPAddress>> blockedNeighbors;
    for (const auto& iter : *(getBlockNeighbors())) {
      auto blockedVlanID = VlanID(
          iter->cref<switch_state_tags::blockNeighborVlanID>()->toThrift());
      auto blockedNeighborIP = network::toIPAddress(
          iter->cref<switch_state_tags::blockNeighborIP>()->toThrift());
      blockedNeighbors.emplace_back(blockedVlanID, blockedNeighborIP);
    }
    return blockedNeighbors;
  }

  void setBlockNeighbors(
      const std::vector<std::pair<VlanID, folly::IPAddress>>& blockNeighbors) {
    std::vector<state::BlockedNeighbor> neighbors{};
    for (auto& entry : blockNeighbors) {
      state::BlockedNeighbor neighbor;
      neighbor.blockNeighborVlanID_ref() = entry.first;
      neighbor.blockNeighborIP_ref() = network::toBinaryAddress(entry.second);
      neighbors.push_back(neighbor);
    }
    setBlockNeighbors(neighbors);
  }

  void setBlockNeighbors(const std::vector<state::BlockedNeighbor>& neighbors) {
    set<switch_state_tags::blockNeighbors>(neighbors);
  }

  auto getMacAddrsToBlock() const {
    return safe_cref<switch_state_tags::macAddrsToBlock>();
  }

  auto getVendorMacOuis() const {
    return safe_cref<switch_state_tags::vendorMacOuis>();
  }

  auto getMetaMacOuis() const {
    return safe_cref<switch_state_tags::metaMacOuis>();
  }

  auto getHostname() const {
    return safe_cref<switch_state_tags::hostname>().get();
  }

  // THRIFT_COPY
  std::vector<std::pair<VlanID, folly::MacAddress>>
  getMacAddrsToBlock_DEPRECATED() const {
    std::vector<std::pair<VlanID, folly::MacAddress>> macAddrs{};
    for (const auto& iter : *(getMacAddrsToBlock())) {
      auto blockedVlanID = VlanID(
          iter->cref<switch_state_tags::macAddrToBlockVlanID>()->toThrift());
      auto blockedMac = folly::MacAddress(
          iter->cref<switch_state_tags::macAddrToBlockAddr>()->toThrift());
      macAddrs.emplace_back(blockedVlanID, blockedMac);
    }
    return macAddrs;
  }

  void setMacAddrsToBlock(
      const std::vector<std::pair<VlanID, folly::MacAddress>>&
          macAddrsToBlock) {
    std::vector<state::BlockedMacAddress> macAddrs{};
    for (auto& entry : macAddrsToBlock) {
      state::BlockedMacAddress macAddr;
      macAddr.macAddrToBlockVlanID_ref() = entry.first;
      macAddr.macAddrToBlockAddr_ref() = entry.second.toString();
      macAddrs.push_back(macAddr);
    }
    set<switch_state_tags::macAddrsToBlock>(macAddrs);
  }

  void setVendorMacOuis(const std::vector<std::string>& vendorMacOuis) {
    set<switch_state_tags::vendorMacOuis>(vendorMacOuis);
  }

  void setMetaMacOuis(const std::vector<std::string>& metaMacOuis) {
    set<switch_state_tags::metaMacOuis>(metaMacOuis);
  }

  void setHostname(const std::string& hostname) {
    set<switch_state_tags::hostname>(hostname);
  }

  cfg::SwitchType getSwitchType(int64_t switchId) const {
    auto switchIdToSwitchInfo = getSwitchIdToSwitchInfo();
    auto iter = switchIdToSwitchInfo.find(switchId);
    if (iter != switchIdToSwitchInfo.end()) {
      return *iter->second.switchType();
    }
    throw FbossError("No SwitchType configured for switchId ", switchId);
  }

  std::unordered_set<SwitchID> getSwitchIds() const;
  std::unordered_set<SwitchID> getSwitchIdsOfType(cfg::SwitchType type) const;
  bool vlansSupported() const;

  bool isSwitchDrained() const {
    return getActualSwitchDrainState() == cfg::SwitchDrainState::DRAINED;
  }

  cfg::SwitchDrainState getSwitchDrainState() const {
    return cref<switch_state_tags::switchDrainState>()->toThrift();
  }

  void setSwitchDrainState(cfg::SwitchDrainState switchDrainState) {
    set<switch_state_tags::switchDrainState>(switchDrainState);
  }

  cfg::SwitchDrainState getActualSwitchDrainState() const {
    return cref<switch_state_tags::actualSwitchDrainState>()->toThrift();
  }

  void setActualSwitchDrainState(cfg::SwitchDrainState actualSwitchDrainState) {
    set<switch_state_tags::actualSwitchDrainState>(actualSwitchDrainState);
  }

  auto getExactMatchTableConfig() const {
    return safe_cref<switch_state_tags::exactMatchTableConfigs>();
  }

  void setExactMatchTableConfig(
      const std::vector<cfg::ExactMatchTableConfig>& exactMatchConfigs) {
    set<switch_state_tags::exactMatchTableConfigs>(exactMatchConfigs);
  }

  std::optional<int32_t> getMinLinksToRemainInVOQDomain() const {
    if (auto minLinksToRemainInVOQDomain =
            cref<switch_state_tags::minLinksToRemainInVOQDomain>()) {
      return minLinksToRemainInVOQDomain->toThrift();
    }
    return std::nullopt;
  }

  void setMinLinksToRemainInVOQDomain(
      std::optional<int64_t> minLinksToRemainInVOQDomain) {
    if (!minLinksToRemainInVOQDomain) {
      ref<switch_state_tags::minLinksToRemainInVOQDomain>().reset();
    } else {
      set<switch_state_tags::minLinksToRemainInVOQDomain>(
          *minLinksToRemainInVOQDomain);
    }
  }

  std::optional<int32_t> getMinLinksToJoinVOQDomain() const {
    if (auto minLinksToJoinVOQDomain =
            cref<switch_state_tags::minLinksToJoinVOQDomain>()) {
      return minLinksToJoinVOQDomain->toThrift();
    }
    return std::nullopt;
  }

  void setMinLinksToJoinVOQDomain(
      std::optional<int64_t> minLinksToJoinVOQDomain) {
    if (!minLinksToJoinVOQDomain) {
      ref<switch_state_tags::minLinksToJoinVOQDomain>().reset();
    } else {
      set<switch_state_tags::minLinksToJoinVOQDomain>(*minLinksToJoinVOQDomain);
    }
  }

  /*
   * 0 or more l3 switch types {NPU, VOQ} are supported on
   * a CPU-NPUs complex
   */
  std::optional<cfg::SwitchType> l3SwitchType() const;
  std::optional<int64_t> getDefaultVlan() const {
    if (auto defaultVlan = cref<switch_state_tags::defaultVlan>()) {
      return defaultVlan->toThrift();
    }
    return std::nullopt;
  }

  void setDefaultVlan(std::optional<int64_t> defaultVlan) {
    if (!defaultVlan) {
      ref<switch_state_tags::defaultVlan>().reset();
    } else {
      set<switch_state_tags::defaultVlan>(*defaultVlan);
    }
  }

  std::optional<std::chrono::seconds> getArpTimeout() const {
    if (auto arpTimeout = cref<switch_state_tags::arpTimeout>()) {
      return std::chrono::seconds(arpTimeout->toThrift());
    }
    return std::nullopt;
  }

  void setArpTimeout(std::optional<std::chrono::seconds> arpTimeout) {
    if (!arpTimeout) {
      ref<switch_state_tags::arpTimeout>().reset();
    } else {
      set<switch_state_tags::arpTimeout>(arpTimeout.value().count());
    }
  }

  std::optional<std::chrono::seconds> getNdpTimeout() const {
    if (auto ndpTimeout = cref<switch_state_tags::ndpTimeout>()) {
      return std::chrono::seconds(ndpTimeout->toThrift());
    }
    return std::nullopt;
  }

  void setNdpTimeout(std::optional<std::chrono::seconds> ndpTimeout) {
    if (!ndpTimeout) {
      ref<switch_state_tags::ndpTimeout>().reset();
    } else {
      set<switch_state_tags::ndpTimeout>(ndpTimeout.value().count());
    }
  }

  std::optional<std::chrono::seconds> getArpAgerInterval() const {
    if (auto arpAgerInterval = cref<switch_state_tags::arpAgerInterval>()) {
      return std::chrono::seconds(arpAgerInterval->toThrift());
    }
    return std::nullopt;
  }

  void setArpAgerInterval(std::optional<std::chrono::seconds> arpAgerInterval) {
    if (!arpAgerInterval) {
      ref<switch_state_tags::arpAgerInterval>().reset();
    } else {
      set<switch_state_tags::arpAgerInterval>(arpAgerInterval.value().count());
    }
  }

  std::optional<std::chrono::seconds> getStaleEntryInterval() const {
    if (auto staleEntryInterval =
            cref<switch_state_tags::staleEntryInterval>()) {
      return std::chrono::seconds(staleEntryInterval->toThrift());
    }
    return std::nullopt;
  }

  void setStaleEntryInterval(
      std::optional<std::chrono::seconds> staleEntryInterval) {
    if (!staleEntryInterval) {
      ref<switch_state_tags::staleEntryInterval>().reset();
    } else {
      set<switch_state_tags::staleEntryInterval>(
          staleEntryInterval.value().count());
    }
  }

  std::optional<int32_t> getMaxNeighborProbes() const {
    if (auto maxNeighborProbes = cref<switch_state_tags::maxNeighborProbes>()) {
      return maxNeighborProbes->toThrift();
    }
    return std::nullopt;
  }

  void setMaxNeighborProbes(std::optional<int32_t> maxNeighborProbes) {
    if (!maxNeighborProbes) {
      ref<switch_state_tags::maxNeighborProbes>().reset();
    } else {
      set<switch_state_tags::maxNeighborProbes>(maxNeighborProbes.value());
    }
  }

  std::optional<folly::IPAddressV4> getDhcpV4RelaySrc() const {
    if (auto dhcpV4RelaySrc = cref<switch_state_tags::dhcpV4RelaySrc>()) {
      return network::toIPAddress(dhcpV4RelaySrc->toThrift()).asV4();
    }
    return std::nullopt;
  }

  void setDhcpV4RelaySrc(std::optional<folly::IPAddressV4> dhcpV4RelaySrc) {
    if (!dhcpV4RelaySrc) {
      ref<switch_state_tags::dhcpV4RelaySrc>().reset();
    } else {
      set<switch_state_tags::dhcpV4RelaySrc>(
          network::toBinaryAddress(*dhcpV4RelaySrc));
    }
  }

  std::optional<folly::IPAddressV6> getDhcpV6RelaySrc() const {
    if (auto dhcpV6RelaySrc = cref<switch_state_tags::dhcpV6RelaySrc>()) {
      return network::toIPAddress(dhcpV6RelaySrc->toThrift()).asV6();
    }
    return std::nullopt;
  }

  void setDhcpV6RelaySrc(std::optional<folly::IPAddressV6> dhcpV6RelaySrc) {
    if (!dhcpV6RelaySrc) {
      ref<switch_state_tags::dhcpV6RelaySrc>().reset();
    } else {
      set<switch_state_tags::dhcpV6RelaySrc>(
          network::toBinaryAddress(*dhcpV6RelaySrc));
    }
  }

  SwitchRunState getSwSwitchRunState() const {
    return cref<switch_state_tags::swSwitchRunState>()->toThrift();
  }

  void setSwSwitchRunState(const SwitchRunState runState) {
    set<switch_state_tags::swSwitchRunState>(runState);
  }

  std::optional<folly::IPAddressV4> getDhcpV4ReplySrc() const {
    if (auto dhcpV4ReplySrc = cref<switch_state_tags::dhcpV4ReplySrc>()) {
      return network::toIPAddress(dhcpV4ReplySrc->toThrift()).asV4();
    }
    return std::nullopt;
  }

  void setDhcpV4ReplySrc(std::optional<folly::IPAddressV4> dhcpV4ReplySrc) {
    if (!dhcpV4ReplySrc) {
      ref<switch_state_tags::dhcpV4ReplySrc>().reset();
    } else {
      set<switch_state_tags::dhcpV4ReplySrc>(
          network::toBinaryAddress(*dhcpV4ReplySrc));
    }
  }

  std::optional<folly::IPAddressV6> getDhcpV6ReplySrc() const {
    if (auto dhcpV6ReplySrc = cref<switch_state_tags::dhcpV6ReplySrc>()) {
      return network::toIPAddress(dhcpV6ReplySrc->toThrift()).asV6();
    }
    return std::nullopt;
  }

  void setDhcpV6ReplySrc(std::optional<folly::IPAddressV6> dhcpV6ReplySrc) {
    if (!dhcpV6ReplySrc) {
      ref<switch_state_tags::dhcpV6ReplySrc>().reset();
    } else {
      set<switch_state_tags::dhcpV6ReplySrc>(
          network::toBinaryAddress(*dhcpV6ReplySrc));
    }
  }

  folly::IPAddressV4 getIcmpV4UnavailableSrcAddress() const {
    auto srcAddress =
        cref<switch_state_tags::icmpV4UnavailableSrcAddress>()->toThrift();
    if (srcAddress.addr()->size() == 0) {
      // RFC7600 - IANA allocated IPv4 dummy address for 192.0.0.8/32
      // Should use this if not set
      return folly::IPAddressV4({192, 0, 0, 8});
    }

    return network::toIPAddress(srcAddress).asV4();
  }

  void setIcmpV4UnavailableSrcAddress(
      folly::IPAddressV4 icmpV4UnavailableSrcAddress) {
    set<switch_state_tags::icmpV4UnavailableSrcAddress>(
        network::toBinaryAddress(icmpV4UnavailableSrcAddress));
  }

  std::shared_ptr<QcmCfg> getQcmCfg() const {
    return safe_cref<switch_state_tags::qcmCfg>();
  }

  void setQcmCfg(std::shared_ptr<QcmCfg> qcmCfg) {
    ref<switch_state_tags::qcmCfg>() = qcmCfg;
  }

  const std::shared_ptr<QosPolicy> getDefaultDataPlaneQosPolicy() const {
    return safe_cref<switch_state_tags::defaultDataPlaneQosPolicy>();
  }

  void setDefaultDataPlaneQosPolicy(std::shared_ptr<QosPolicy> qosPolicy) {
    ref<switch_state_tags::defaultDataPlaneQosPolicy>() = qosPolicy;
  }

  const std::shared_ptr<UdfConfig> getUdfConfig() const {
    return safe_cref<switch_state_tags::udfConfig>();
  }

  void setUdfConfig(std::shared_ptr<UdfConfig> udfConfig) {
    ref<switch_state_tags::udfConfig>() = udfConfig;
  }

  const std::shared_ptr<FlowletSwitchingConfig> getFlowletSwitchingConfig()
      const {
    return safe_cref<switch_state_tags::flowletSwitchingConfig>();
  }

  void setFlowletSwitchingConfig(
      std::shared_ptr<FlowletSwitchingConfig> flowletConfig) {
    ref<switch_state_tags::flowletSwitchingConfig>() = flowletConfig;
  }

  SwitchIdToSwitchInfo getSwitchIdToSwitchInfo() const {
    // THRIFT_COPY
    return get<switch_state_tags::switchIdToSwitchInfo>()->toThrift();
  }

  void setSwitchIdToSwitchInfo(const SwitchIdToSwitchInfo& switchInfo) {
    set<switch_state_tags::switchIdToSwitchInfo>(switchInfo);
  }

  void setDefaultVoqConfig(const QueueConfig& queues) {
    std::vector<PortQueueFields> queuesThrift{};
    for (const auto& queue : queues) {
      queuesThrift.push_back(queue->toThrift());
    }
    set<switch_state_tags::defaultVoqConfig>(std::move(queuesThrift));
  }

  const QueueConfig& getDefaultVoqConfig() const {
    const auto& queues = cref<switch_state_tags::defaultVoqConfig>();
    return queues->impl();
  }

  void setSwitchInfo(const cfg::SwitchInfo& switchInfo) {
    set<switch_state_tags::switchInfo>(switchInfo);
  }

  const cfg::SwitchInfo getSwitchInfo() const {
    return get<switch_state_tags::switchInfo>()->toThrift();
  }

  std::optional<bool> getForceTrafficOverFabric() const {
    if (auto forceTrafficOverFabric =
            cref<switch_state_tags::forceTrafficOverFabric>()) {
      return forceTrafficOverFabric->toThrift();
    }
    return std::nullopt;
  }

  void setForceTrafficOverFabric(std::optional<bool> forceTrafficOverFabric) {
    if (!forceTrafficOverFabric) {
      ref<switch_state_tags::forceTrafficOverFabric>().reset();
    } else {
      set<switch_state_tags::forceTrafficOverFabric>(
          forceTrafficOverFabric.value());
    }
  }

  std::optional<bool> getCreditWatchdog() const {
    if (auto creditWatchdog = cref<switch_state_tags::creditWatchdog>()) {
      return creditWatchdog->toThrift();
    }
    return std::nullopt;
  }

  void setCreditWatchdog(std::optional<bool> creditWatchdog) {
    if (!creditWatchdog) {
      ref<switch_state_tags::creditWatchdog>().reset();
    } else {
      set<switch_state_tags::creditWatchdog>(creditWatchdog.value());
    }
  }

  std::optional<bool> getForceEcmpDynamicMemberUp() const {
    if (auto forceEcmpDynamicMemberUp =
            cref<switch_state_tags::forceEcmpDynamicMemberUp>()) {
      return forceEcmpDynamicMemberUp->toThrift();
    }
    return std::nullopt;
  }

  void setForceEcmpDynamicMemberUp(
      std::optional<bool> forceEcmpDynamicMemberUp) {
    if (!forceEcmpDynamicMemberUp) {
      ref<switch_state_tags::forceEcmpDynamicMemberUp>().reset();
    } else {
      set<switch_state_tags::forceEcmpDynamicMemberUp>(
          forceEcmpDynamicMemberUp.value());
    }
  }

  std::optional<int32_t> getReachabilityGroupListSize() const {
    if (auto reachabilityGroupListSize =
            cref<switch_state_tags::reachabilityGroupListSize>()) {
      return reachabilityGroupListSize->toThrift();
    }
    return std::nullopt;
  }

  void setReachabilityGroupListSize(
      std::optional<int32_t> reachabilityGroupListSize) {
    if (!reachabilityGroupListSize) {
      ref<switch_state_tags::reachabilityGroupListSize>().reset();
    } else {
      set<switch_state_tags::reachabilityGroupListSize>(
          *reachabilityGroupListSize);
    }
  }

  SwitchSettings* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

using MultiSwitchSettingsTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using MultiSwitchSettingsThriftType =
    std::map<std::string, state::SwitchSettingsFields>;

class MultiSwitchSettings;

using MultiSwitchSettingsTraits = ThriftMapNodeTraits<
    MultiSwitchSettings,
    MultiSwitchSettingsTypeClass,
    MultiSwitchSettingsThriftType,
    SwitchSettings>;

class HwSwitchMatcher;

class MultiSwitchSettings
    : public ThriftMapNode<MultiSwitchSettings, MultiSwitchSettingsTraits> {
 public:
  using Traits = MultiSwitchSettingsTraits;
  using BaseT = ThriftMapNode<MultiSwitchSettings, MultiSwitchSettingsTraits>;
  using BaseT::modify;

  std::shared_ptr<SwitchSettings> getSwitchSettings(
      const HwSwitchMatcher& matcher) const {
    return getNodeIf(matcher.matcherString());
  }
  MultiSwitchSettings() = default;
  virtual ~MultiSwitchSettings() = default;

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
