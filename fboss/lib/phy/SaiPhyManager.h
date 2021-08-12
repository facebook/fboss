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
#include "fboss/mka_service/if/gen-cpp2/mka_types.h"

#include "fboss/agent/types.h"

#include <map>

namespace facebook::fboss {

class SaiHwPlatform;

class SaiPhyManager : public PhyManager {
 public:
  explicit SaiPhyManager(const PlatformMapping* platformMapping);
  ~SaiPhyManager() override;

  SaiHwPlatform* getSaiPlatform(GlobalXphyID xphyID) const;
  SaiHwPlatform* getSaiPlatform(PortID portID) const;

  SaiSwitch* getSaiSwitch(GlobalXphyID xphyID) const;
  SaiSwitch* getSaiSwitch(PortID portID) const;

  void sakInstallTx(const mka::MKASak& sak) override;
  void sakInstallRx(const mka::MKASak& sak, const mka::MKASci& sci) override;
  void sakDeleteRx(const mka::MKASak& sak, const mka::MKASci& sci) override;
  void sakDelete(const mka::MKASak& sak) override;

  void programOnePort(
      PortID portId,
      cfg::PortProfileID portProfileId,
      std::optional<TransceiverInfo> transceiverInfo) override;

  template <typename platformT, typename xphychipT>
  void initializeSlotPhysImpl(PimID pimID);

  PortOperState macsecGetPhyLinkInfo(PortID swPort);

 protected:
  void addSaiPlatform(
      GlobalXphyID xphyID,
      std::unique_ptr<SaiHwPlatform> platform);

 private:
  // Forbidden copy constructor and assignment operator
  SaiPhyManager(SaiPhyManager const&) = delete;
  SaiPhyManager& operator=(SaiPhyManager const&) = delete;

  SaiMacsecManager* getMacsecManager(PortID portId);
  PortID getPortId(std::string portName);

  std::unique_ptr<ExternalPhyPortStatsUtils> createExternalPhyPortStats(
      PortID portID) override;

  std::map<PimID, std::map<GlobalXphyID, std::unique_ptr<SaiHwPlatform>>>
      saiPlatforms_;
};

template <typename platformT, typename xphychipT>
void SaiPhyManager::initializeSlotPhysImpl(PimID pimID) {
  if (const auto pimPhyMap = xphyMap_.find(pimID);
      pimPhyMap != xphyMap_.end()) {
    for (const auto& phy : pimPhyMap->second) {
      auto saiPlatform = static_cast<platformT*>(getSaiPlatform(phy.first));

      XLOG(DBG2) << "About to initialize phy of global phyId:" << phy.first;
      // Create CredoF104 sai switch
      auto credoF104 = static_cast<xphychipT*>(getExternalPhy(phy.first));
      // Set CredoF104's customized switch attributes before calling init
      saiPlatform->setSwitchAttributes(credoF104->getSwitchAttributes());
      saiPlatform->init(
          nullptr /* No AgentConfig needed */,
          0 /* No switch featured needed */);

      // Now call HwSwitch to create the switch object in hardware
      auto saiSwitch = static_cast<SaiSwitch*>(saiPlatform->getHwSwitch());
      saiSwitch->init(credoF104, true /* failHwCallsOnWarmboot */);
      credoF104->setSwitchId(saiSwitch->getSwitchId());
      credoF104->dump();
    }
  }
}
} // namespace facebook::fboss
