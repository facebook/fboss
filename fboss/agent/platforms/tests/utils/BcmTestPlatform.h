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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/platforms/tests/utils/BcmTestPort.h"
#include "fboss/agent/platforms/tests/utils/TestPlatformTypes.h"
#include "fboss/agent/types.h"

#include <folly/MacAddress.h>

#include <map>
#include <memory>
#include <vector>

namespace facebook::fboss {

class PlatformProductInfo;
class BcmSwitch;

class BcmTestPlatform : public BcmPlatform {
 public:
  BcmTestPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<PlatformMapping> platformMapping);
  ~BcmTestPlatform() override;

  HwSwitch* getHwSwitch() const override;
  void onHwInitialized(SwSwitch* sw) override;
  void onInitialConfigApplied(SwSwitch* sw) override;
  void stop() override;
  std::unique_ptr<ThriftHandler> createHandler(SwSwitch* sw) override;

  virtual std::vector<FlexPortMode> getSupportedFlexPortModes() const = 0;

  void onUnitCreate(int unit) override;
  void onUnitAttach(int unit) override;
  void initPorts() override;
  BcmPlatformPortMap getPlatformPortMap() override;

  bool canUseHostTableForHostRoutes() const override {
    return true;
  }
  TransceiverIdxThrift getPortMapping(
      PortID /* unused */,
      cfg::PortSpeed /* speed */) const override {
    return TransceiverIdxThrift();
  }
  bool isBcmShellSupported() const override {
    return true;
  }

  const std::vector<PortID>& logicalPortIds() const {
    return logicalPortIds_;
  }
  const std::vector<PortID>& masterLogicalPortIds() const {
    return masterLogicalPortIds_;
  }
  std::vector<PortID> getAllPortsInGroup(PortID portID) const;

  BcmWarmBootHelper* getWarmBootHelper() override {
    return warmBootHelper_.get();
  }

  PlatformPort* getPlatformPort(PortID portID) const override;

  void initLEDs(int unit, folly::ByteRange led0, folly::ByteRange led1);

  virtual void initLEDs(int /*unit*/) {}

  virtual bool verifyLEDStatus(PortID /*port*/, bool /*up*/) {
    return true;
  }

  virtual bool usesYamlConfig() const {
    return false;
  }

  void setOverridePortInterPacketGapBits(uint32_t ipgBits) {
    overridePortInterPacketGapBits_ = ipgBits;
  }

  const std::optional<phy::PortProfileConfig> getPortProfileConfig(
      PlatformPortProfileConfigMatcher profileMatcher) const override;

 protected:
  // Each platform should have their own logical ports list.
  std::vector<PortID> logicalPortIds_;
  // List of controlling ports on platform. Each of these then
  // have subports that can be used when using flex ports
  std::vector<PortID> masterLogicalPortIds_;
  int numPortsPerTranceiver_;

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestPlatform(BcmTestPlatform const&) = delete;
  BcmTestPlatform& operator=(BcmTestPlatform const&) = delete;

  void initImpl(uint32_t hwFeaturesDesired) override;
  virtual std::unique_ptr<BcmTestPort> createTestPort(PortID portID) = 0;

  std::map<PortID, std::unique_ptr<BcmTestPort>> ports_;
  std::unique_ptr<BcmWarmBootHelper> warmBootHelper_;
  std::unique_ptr<BcmSwitch> bcmSwitch_;
  std::optional<uint32_t> overridePortInterPacketGapBits_;
};

} // namespace facebook::fboss
