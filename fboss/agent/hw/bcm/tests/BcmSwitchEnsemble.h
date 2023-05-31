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

#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/platforms/tests/utils/BcmTestPlatform.h"

DECLARE_bool(flexports);
DECLARE_string(bcm_config);
DECLARE_string(config);
DECLARE_string(mode);

namespace facebook::fboss {

class BcmSwitchEnsemble : public HwSwitchEnsemble {
 public:
  explicit BcmSwitchEnsemble(
      const HwSwitchEnsemble::Features& features = {
          HwSwitchEnsemble::PACKET_RX,
          HwSwitchEnsemble::LINKSCAN});
  BcmTestPlatform* getPlatform() override {
    return static_cast<BcmTestPlatform*>(HwSwitchEnsemble::getPlatform());
  }
  const BcmTestPlatform* getPlatform() const override {
    return static_cast<const BcmTestPlatform*>(HwSwitchEnsemble::getPlatform());
  }
  BcmSwitch* getHwSwitch() override {
    return static_cast<BcmSwitch*>(HwSwitchEnsemble::getHwSwitch());
  }
  const BcmSwitch* getHwSwitch() const override {
    return static_cast<const BcmSwitch*>(HwSwitchEnsemble::getHwSwitch());
  }
  std::vector<PortID> masterLogicalPortIds() const override;
  std::vector<PortID> getAllPortsInGroup(PortID portID) const override;
  std::vector<FlexPortMode> getSupportedFlexPortModes() const override;
  uint64_t getSdkSwitchId() const override;
  bool isRouteScaleEnabled() const override;

  void dumpHwCounters() const override;

  std::map<PortID, HwPortStats> getLatestPortStats(
      const std::vector<PortID>& ports) override;

  std::map<AggregatePortID, HwTrunkStats> getLatestAggregatePortStats(
      const std::vector<AggregatePortID>& aggregatePorts) override;

  void runDiagCommand(const std::string& input, std::string& output) override;

  void init(const HwSwitchEnsemble::HwSwitchEnsembleInitInfo& info) override;

  void createEqualDistributedUplinkDownlinks(
      const std::vector<PortID>& ports,
      std::vector<PortID>& uplinks,
      std::vector<PortID>& downlinks,
      std::vector<PortID>& disabled,
      const int totalLinktCount) override;

  bool isSai() const override {
    return false;
  }

 private:
  std::unique_ptr<std::thread> setupThrift() override {
    return createThriftThread(getHwSwitch());
  }
  std::unique_ptr<HwLinkStateToggler> createLinkToggler(
      HwSwitch* hwSwitch,
      const std::map<cfg::PortType, cfg::PortLoopbackMode>&
          desiredLoopbackModes);
  std::unique_ptr<std::thread> createThriftThread(const BcmSwitch* hwSwitch);
};

} // namespace facebook::fboss
