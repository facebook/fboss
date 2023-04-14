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

#include "fboss/agent/hw/sai/switch/SaiMacsecManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/mka_service/if/gen-cpp2/mka_structs_types.h"

#include "fboss/agent/types.h"

#include <chrono>
#include <map>
#include <mutex>

namespace facebook::fboss {

class SaiHwPlatform;
class StateUpdate;
class Port;

class SaiPhyManager : public PhyManager {
 public:
  explicit SaiPhyManager(const PlatformMapping* platformMapping);
  ~SaiPhyManager() override;

  SaiHwPlatform* getSaiPlatform(GlobalXphyID xphyID);
  const SaiHwPlatform* getSaiPlatform(GlobalXphyID xphyID) const {
    return const_cast<SaiPhyManager*>(this)->getSaiPlatform(xphyID);
  }
  SaiHwPlatform* getSaiPlatform(PortID portID);
  const SaiHwPlatform* getSaiPlatform(PortID portID) const {
    return const_cast<SaiPhyManager*>(this)->getSaiPlatform(portID);
  }
  SaiSwitch* getSaiSwitch(GlobalXphyID xphyID);
  const SaiSwitch* getSaiSwitch(GlobalXphyID xphyID) const {
    return const_cast<SaiPhyManager*>(this)->getSaiSwitch(xphyID);
  }
  SaiSwitch* getSaiSwitch(PortID portID);
  const SaiSwitch* getSaiSwitch(PortID portID) const {
    return const_cast<SaiPhyManager*>(this)->getSaiSwitch(portID);
  }

  void sakInstallTx(const mka::MKASak& sak) override;
  void sakInstallRx(const mka::MKASak& sak, const mka::MKASci& sci) override;
  void sakDeleteRx(const mka::MKASak& sak, const mka::MKASci& sci) override;
  void sakDelete(const mka::MKASak& sak) override;
  mka::MKASakHealthResponse sakHealthCheck(
      const mka::MKASak sak) const override;

  mka::MacsecPortStats getMacsecPortStats(
      std::string portName,
      mka::MacsecDirection direction,
      bool readFromHw) override;
  mka::MacsecFlowStats getMacsecFlowStats(
      std::string portName,
      mka::MacsecDirection direction,
      bool readFromHw) override;
  mka::MacsecSaStats getMacsecSecureAssocStats(
      std::string portName,
      mka::MacsecDirection direction,
      bool readFromHw) override;

  std::map<std::string, MacsecStats> getAllMacsecPortStats(
      bool readFromHw) override;

  std::map<std::string, MacsecStats> getMacsecPortStats(
      const std::vector<std::string>& portName,
      bool readFromHw) override;

  std::string listHwObjects(std::vector<HwObjectType>& hwObjects, bool cached)
      override;

  void programOnePort(
      PortID portId,
      cfg::PortProfileID portProfileId,
      std::optional<TransceiverInfo> transceiverInfo,
      bool needResetDataPath) override;

  template <typename platformT, typename xphychipT>
  void initializeSlotPhysImpl(PimID pimID);

  PortOperState macsecGetPhyLinkInfo(PortID swPort);
  phy::PhyInfo getPhyInfo(PortID swPort) override;
  void updateAllXphyPortsStats() override;

  bool getSdkState(const std::string& fileName) override;

  bool setupMacsecState(
      const std::vector<std::string>& portList,
      bool macsecDesired,
      bool dropUnencrypted) override;

  void setPortPrbs(
      PortID portID,
      phy::Side side,
      const phy::PortPrbsState& prbs) override;

  phy::PortPrbsState getPortPrbs(PortID portID, phy::Side side) override;

  std::vector<phy::PrbsLaneStats> getPortPrbsStats(
      PortID portID,
      phy::Side side) override;

  void setPortLoopbackState(
      PortID swPort,
      phy::PortComponent component,
      bool setLoopback) override {
    setSaiPortLoopbackState(swPort, component, setLoopback);
  }

  void setPortAdminState(
      PortID swPort,
      phy::PortComponent component,
      bool setAdminUp) override {
    setSaiPortAdminState(swPort, component, setAdminUp);
  }

  std::string getPortInfoStr(PortID swPort) override {
    return getSaiPortInfo(swPort);
  }

  void xphyPortStateToggle(PortID swPort, phy::Side side);

  void gracefulExit() override;

 protected:
  void addSaiPlatform(
      GlobalXphyID xphyID,
      std::unique_ptr<SaiHwPlatform> platform);

  folly::MacAddress getLocalMac() const {
    return localMac_;
  }

 private:
  std::shared_ptr<SwitchState> portUpdateHelper(
      std::shared_ptr<SwitchState> in,
      PortID port,
      const SaiHwPlatform* platform,
      const std::function<void(std::shared_ptr<Port>&)>& modify) const;
  class PlatformInfo {
   public:
    explicit PlatformInfo(std::unique_ptr<SaiHwPlatform> platform);
    PlatformInfo(PlatformInfo&&) = default;
    PlatformInfo& operator=(PlatformInfo&&) = default;
    ~PlatformInfo();

    using StateUpdateFn = std::function<std::shared_ptr<SwitchState>(
        const std::shared_ptr<SwitchState>&)>;

    SaiSwitch* getHwSwitch();
    SaiHwPlatform* getPlatform() {
      return saiPlatform_.get();
    }
    void applyUpdate(folly::StringPiece name, StateUpdateFn fn);

    std::shared_ptr<SwitchState> getState() const {
      return *appliedStateDontUseDirectly_.rlock();
    }

   private:
    void setState(const std::shared_ptr<SwitchState>& newState);
    std::unique_ptr<SaiHwPlatform> saiPlatform_;
    // Don't hold locked access to SwitchState for long periods. Instead
    // Just access via set/getState apis, to allow for many readers just
    // accessing the COW SwitchState object w/o holding a lock.
    folly::Synchronized<std::shared_ptr<SwitchState>>
        appliedStateDontUseDirectly_;
    std::mutex updateMutex_;
  };
  PlatformInfo* getPlatformInfo(GlobalXphyID xphyID);
  const PlatformInfo* getPlatformInfo(GlobalXphyID xphyID) const {
    return const_cast<SaiPhyManager*>(this)->getPlatformInfo(xphyID);
  }
  PlatformInfo* getPlatformInfo(PortID portID);
  const PlatformInfo* getPlatformInfo(PortID portId) const {
    return const_cast<SaiPhyManager*>(this)->getPlatformInfo(portId);
  }
  MacsecStats getMacsecStats(const std::string& portName, bool readFromHw) {
    return readFromHw ? getMacsecStatsFromHw(portName)
                      : getMacsecStats(portName);
  }
  MacsecStats getMacsecStatsFromHw(const std::string& portName);
  MacsecStats getMacsecStats(const std::string& portName) const;
  // Forbidden copy constructor and assignment operator
  SaiPhyManager(SaiPhyManager const&) = delete;
  SaiPhyManager& operator=(SaiPhyManager const&) = delete;

  PortID getPortId(std::string portName) const;

  std::unique_ptr<ExternalPhyPortStatsUtils> createExternalPhyPortStats(
      PortID portID) override;

  virtual bool isPortCreateAllowed(
      GlobalXphyID /* globalXphyId */,
      const phy::PhyPortConfig& /* portCfg */) {
    return true;
  }

  void setSaiPortLoopbackState(
      PortID swPort,
      phy::PortComponent component,
      bool setLoopback);

  void setSaiPortAdminState(
      PortID swPort,
      phy::PortComponent component,
      bool setAdminUp);

  std::string getSaiPortInfo(PortID swPort);

  // Due to SaiPhyManager usually has more than one phy, and each phy has its
  // own SaiHwPlatform, which needs a local mac address. As local mac address
  // will be the same mac address for the running system, all these phys and
  // their Platforms will share the same local mac.
  // Therefore, to avoid calling the getLocalMacAddress() too frequently,
  // use a const private member to store the local mac once, and then pass this
  // mac address when creating each single SaiHwPlatform
  const folly::MacAddress localMac_;
  std::map<PimID, std::map<GlobalXphyID, std::unique_ptr<PlatformInfo>>>
      saiPlatforms_;
  std::map<PimID, std::optional<folly::Future<folly::Unit>>>
      pim2OngoingStatsCollection_;
};

using namespace std::chrono;
template <typename platformT, typename xphychipT>
void SaiPhyManager::initializeSlotPhysImpl(PimID pimID) {
  if (const auto pimPhyMap = xphyMap_.find(pimID);
      pimPhyMap != xphyMap_.end()) {
    for (const auto& phy : pimPhyMap->second) {
      auto saiPlatform = static_cast<platformT*>(getSaiPlatform(phy.first));

      XLOG(DBG2) << "About to initialize phy of global phyId:" << phy.first;
      steady_clock::time_point begin = steady_clock::now();
      // Create xphy sai switch
      auto xphy = static_cast<xphychipT*>(getExternalPhy(phy.first));
      // Set xphy's customized switch attributes before calling init
      saiPlatform->setSwitchAttributes(xphy->getSwitchAttributes());
      cfg::AgentConfig config;
      cfg::SwitchInfo switchInfo;
      switchInfo.switchType() = cfg::SwitchType::PHY;
      switchInfo.asicType() = saiPlatform->getAsic()->getAsicType();
      config.sw()->switchSettings()->switchIdToSwitchInfo() = {
          std::make_pair(0, switchInfo)};
      saiPlatform->init(
          std::make_unique<AgentConfig>(config, ""),
          0 /* No switch featured needed */);

      // Now call HwSwitch to create the switch object in hardware
      auto saiSwitch = static_cast<SaiSwitch*>(saiPlatform->getHwSwitch());
      saiSwitch->init(xphy, true /* failHwCallsOnWarmboot */);
      xphy->setSwitchId(saiSwitch->getSaiSwitchId());
      xphy->dump();
      XLOG(DBG2)
          << "Finished initializing phy of global phyId:" << phy.first
          << ", switchId:" << saiSwitch->getSaiSwitchId() << " took "
          << duration_cast<milliseconds>(steady_clock::now() - begin).count()
          << "ms";
    }
  }
}
} // namespace facebook::fboss
