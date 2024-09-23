// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/state/StateUpdateHelpers.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <set>
#include <vector>

namespace facebook::fboss {

class SwitchState;
class SwitchIdScopeResolver;
class RouteUpdateWrapper;
class LinkStateToggler;

class TestEnsembleIf : public HwSwitchCallback {
 public:
  using StateUpdateFn = FunctionStateUpdate::StateUpdateFn;
  ~TestEnsembleIf() override {}
  virtual std::vector<PortID> masterLogicalPortIds() const = 0;
  std::vector<PortID> masterLogicalPortIds(
      const std::set<cfg::PortType>& portTypes) const {
    return masterLogicalPortIdsImpl(portTypes, {});
  }
  std::vector<PortID> masterLogicalInterfacePortIds() const {
    return masterLogicalPortIds({cfg::PortType::INTERFACE_PORT});
  }
  std::vector<PortID> masterLogicalFabricPortIds() const {
    return masterLogicalPortIds({cfg::PortType::FABRIC_PORT});
  }

  std::vector<PortID> masterLogicalPortIds(
      const std::set<cfg::PortType>& portTypes,
      const std::set<SwitchID>& switchIds) const {
    return masterLogicalPortIdsImpl(portTypes, switchIds);
  }
  std::vector<PortID> masterLogicalInterfacePortIds(
      const std::set<SwitchID>& switchIds) const {
    return masterLogicalPortIds({cfg::PortType::INTERFACE_PORT}, switchIds);
  }
  std::vector<PortID> masterLogicalFabricPortIds(
      const std::set<SwitchID>& switchIds) const {
    return masterLogicalPortIds({cfg::PortType::FABRIC_PORT}, switchIds);
  }
  std::vector<PortID> masterLogicalPortIds(
      const std::set<cfg::PortType>& portTypes,
      SwitchID switchId) const {
    return masterLogicalPortIdsImpl(portTypes, {switchId});
  }
  std::vector<PortID> masterLogicalInterfacePortIds(SwitchID switchId) const {
    return masterLogicalPortIds({cfg::PortType::INTERFACE_PORT}, {switchId});
  }
  std::vector<PortID> masterLogicalFabricPortIds(SwitchID switchId) const {
    return masterLogicalPortIds({cfg::PortType::FABRIC_PORT}, {switchId});
  }

  size_t getMinPktsForLineRate(const PortID& port) {
    auto portSpeed =
        getProgrammedState()->getPorts()->getNodeIf(port)->getSpeed();
    return (portSpeed > cfg::PortSpeed::HUNDREDG ? 1000 : 100);
  }

  virtual void applyNewState(
      StateUpdateFn fn,
      const std::string& name = "test-update",
      bool rollbackOnHwOverflow = false) = 0;
  virtual void applyInitialConfig(const cfg::SwitchConfig& config) = 0;
  virtual std::shared_ptr<SwitchState> applyNewConfig(
      const cfg::SwitchConfig& config) = 0;
  virtual std::shared_ptr<SwitchState> getProgrammedState() const = 0;
  virtual const std::map<int32_t, cfg::PlatformPortEntry>& getPlatformPorts()
      const = 0;
  virtual void switchRunStateChanged(SwitchRunState runState) = 0;
  virtual const SwitchIdScopeResolver& scopeResolver() const = 0;
  virtual HwAsicTable* getHwAsicTable() = 0;
  virtual const HwAsicTable* getHwAsicTable() const = 0;
  virtual std::map<PortID, FabricEndpoint> getFabricConnectivity(
      SwitchID switchId) const = 0;
  virtual FabricReachabilityStats getFabricReachabilityStats() const = 0;
  virtual void updateStats() = 0;
  virtual void runDiagCommand(
      const std::string& input,
      std::string& output,
      std::optional<SwitchID> switchId = std::nullopt) = 0;
  virtual std::unique_ptr<RouteUpdateWrapper> getRouteUpdaterWrapper() = 0;
  virtual void clearPortStats(
      const std::unique_ptr<std::vector<int32_t>>& ports) = 0;
  virtual std::map<PortID, HwPortStats> getLatestPortStats(
      const std::vector<PortID>& ports) = 0;
  HwPortStats getLatestPortStats(PortID port) {
    return getLatestPortStats(std::vector<PortID>({port}))[port];
  }
  virtual std::map<SystemPortID, HwSysPortStats> getLatestSysPortStats(
      const std::vector<SystemPortID>& ports) = 0;
  HwSysPortStats getLatestSysPortStats(SystemPortID port) {
    return getLatestSysPortStats(std::vector<SystemPortID>({port}))[port];
  }
  virtual LinkStateToggler* getLinkToggler() = 0;
  virtual bool isSai() const = 0;
  virtual folly::MacAddress getLocalMac(SwitchID id) const = 0;
  virtual void sendPacketAsync(
      std::unique_ptr<TxPacket> pkt,
      std::optional<PortDescriptor> portDescriptor = std::nullopt,
      std::optional<uint8_t> queueId = std::nullopt) = 0;
  virtual std::unique_ptr<TxPacket> allocatePacket(uint32_t size) = 0;
  virtual bool supportsAddRemovePort() const = 0;
  virtual const PlatformMapping* getPlatformMapping() const = 0;
  virtual cfg::SwitchConfig getCurrentConfig() const = 0;
  std::vector<const HwAsic*> getL3Asics() const;
  int getNumL3Asics() const;
  std::vector<SystemPortID> masterLogicalSysPortIds() const;

 private:
  std::vector<PortID> masterLogicalPortIdsImpl(
      const std::set<cfg::PortType>& portTypes,
      const std::set<SwitchID>& switches) const;
};

} // namespace facebook::fboss
