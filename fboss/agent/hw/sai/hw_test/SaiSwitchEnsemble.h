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

#include "fboss/agent/hw/test/HwSwitchEnsemble.h"

#include "fboss/agent/hw/sai/diag/DiagShell.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <memory>

namespace facebook::fboss {

class SaiTestHandler;

class SaiSwitchEnsemble : public HwSwitchEnsemble {
 public:
  explicit SaiSwitchEnsemble(
      const HwSwitchEnsemble::Features& features = {
          HwSwitchEnsemble::PACKET_RX,
          HwSwitchEnsemble::LINKSCAN,
          HwSwitchEnsemble::TAM_NOTIFY,
      });
  SaiPlatform* getPlatform() override {
    return static_cast<SaiPlatform*>(HwSwitchEnsemble::getPlatform());
  }
  const SaiPlatform* getPlatform() const override {
    return static_cast<const SaiPlatform*>(HwSwitchEnsemble::getPlatform());
  }
  SaiSwitch* getHwSwitch() override {
    return static_cast<SaiSwitch*>(HwSwitchEnsemble::getHwSwitch());
  }
  const SaiSwitch* getHwSwitch() const override {
    return static_cast<const SaiSwitch*>(HwSwitchEnsemble::getHwSwitch());
  }
  std::vector<PortID> masterLogicalPortIds() const override;
  std::vector<PortID> getAllPortsInGroup(PortID portID) const override;
  std::vector<FlexPortMode> getSupportedFlexPortModes() const override;
  uint64_t getSdkSwitchId() const override;
  /*
   * Sai tests are always run with "route scale" mode on. For Bcm chips
   * this means ALPM mode is on.
   */
  bool isRouteScaleEnabled() const override {
    return true;
  }
  void dumpHwCounters() const override;

  std::map<AggregatePortID, HwTrunkStats> getLatestAggregatePortStats(
      const std::vector<AggregatePortID>& aggregatePorts) override;

  void runDiagCommand(const std::string& input, std::string& output) override;

  void init(const HwSwitchEnsemble::HwSwitchEnsembleInitInfo& info) override;
  void gracefulExit() override;

  bool isSai() const override {
    return true;
  }
  std::map<int64_t, cfg::DsfNode> dsfNodesFromInputConfig() const override;

 private:
  std::unique_ptr<std::thread> setupThrift() override {
    return createThriftThread(getHwSwitch());
  }
  std::unique_ptr<HwLinkStateToggler> createLinkToggler(
      HwSwitch* hwSwitch,
      cfg::PortLoopbackMode desiredLoopbackMode);
  std::unique_ptr<std::thread> createThriftThread(const SaiSwitch* hwSwitch);
  std::unique_ptr<DiagShell> diagShell_;
  std::unique_ptr<DiagCmdServer> diagCmdServer_;
};

} // namespace facebook::fboss
