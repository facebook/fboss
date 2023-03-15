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
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"

namespace facebook::fboss {

class SwitchState;

USE_THRIFT_COW(SwitchSettings)
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
      blockedNeighbors.push_back(
          std::make_pair(blockedVlanID, blockedNeighborIP));
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

  // THRIFT_COPY
  std::vector<std::pair<VlanID, folly::MacAddress>>
  getMacAddrsToBlock_DEPRECATED() const {
    std::vector<std::pair<VlanID, folly::MacAddress>> macAddrs{};
    for (const auto& iter : *(getMacAddrsToBlock())) {
      auto blockedVlanID = VlanID(
          iter->cref<switch_state_tags::macAddrToBlockVlanID>()->toThrift());
      auto blockedMac = folly::MacAddress(
          iter->cref<switch_state_tags::macAddrToBlockAddr>()->toThrift());
      macAddrs.push_back(std::make_pair(blockedVlanID, blockedMac));
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

  cfg::SwitchType getSwitchType() const {
    return cref<switch_state_tags::switchType>()->toThrift();
  }
  void setSwitchType(cfg::SwitchType type) {
    set<switch_state_tags::switchType>(type);
  }

  std::optional<int64_t> getSwitchId() const {
    if (auto switchId = cref<switch_state_tags::switchId>()) {
      return switchId->toThrift();
    }
    return std::nullopt;
  }
  void setSwitchId(std::optional<int64_t> switchId) {
    if (!switchId) {
      ref<switch_state_tags::switchId>().reset();
    } else {
      set<switch_state_tags::switchId>(*switchId);
    }
  }

  auto getExactMatchTableConfig() const {
    return safe_cref<switch_state_tags::exactMatchTableConfigs>();
  }

  void setExactMatchTableConfig(
      const std::vector<cfg::ExactMatchTableConfig>& exactMatchConfigs) {
    set<switch_state_tags::exactMatchTableConfigs>(exactMatchConfigs);
  }

  std::optional<cfg::Range64> getSystemPortRange() const {
    if (auto range = cref<switch_state_tags::systemPortRange>()) {
      return range->toThrift();
    }
    return std::nullopt;
  }
  void setSystemPortRange(std::optional<cfg::Range64> systemPortRange) {
    if (!systemPortRange) {
      ref<switch_state_tags::systemPortRange>().reset();
    } else {
      set<switch_state_tags::systemPortRange>(*systemPortRange);
    }
  }

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

  SwitchSettings* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
