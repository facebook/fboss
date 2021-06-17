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
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/mka_service/if/gen-cpp2/mka_types.h"

#include "fboss/agent/types.h"

#include <map>

namespace facebook::fboss {

class SaiHwPlatform;
class SaiSwitch;

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

  std::map<PimID, std::map<GlobalXphyID, std::unique_ptr<SaiHwPlatform>>>
      saiPlatforms_;
};
} // namespace facebook::fboss
