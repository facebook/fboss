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
#include "fboss/agent/platforms/test_platforms/BcmTestPort.h"
#include "fboss/agent/platforms/test_platforms/TestPlatformTypes.h"
#include "fboss/agent/types.h"

#include <folly/MacAddress.h>

#include <map>
#include <memory>

namespace facebook {
namespace fboss {

class BcmTestPlatform : public BcmPlatform {
 public:
  BcmTestPlatform(
      std::vector<PortID> masterLogicalPortIds,
      int numPortsPerTranceiver);
  ~BcmTestPlatform() override;

  HwSwitch* getHwSwitch() const override;
  void onHwInitialized(SwSwitch* sw) override;
  void onInitialConfigApplied(SwSwitch* sw) override;
  void stop() override;
  std::unique_ptr<ThriftHandler> createHandler(SwSwitch* sw) override;

  folly::MacAddress getLocalMac() const override;
  virtual std::list<FlexPortMode> getSupportedFlexPortModes() const = 0;

  virtual cfg::PortLoopbackMode desiredLoopbackMode() const {
    return cfg::PortLoopbackMode::MAC;
  }

  void onUnitCreate(int unit) override;
  void onUnitAttach(int unit) override;
  InitPortMap initPorts() override;

  void getProductInfo(ProductInfo& /*info*/) override{
      // Nothing to do
  };
  bool canUseHostTableForHostRoutes() const override {
    return true;
  }
  virtual bool hasLinkScanCapability() const = 0;
  TransceiverIdxThrift getPortMapping(PortID /* unused */) const override {
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
  std::vector<PortID> getAllPortsinGroup(PortID portID) const;

  BcmWarmBootHelper* getWarmBootHelper() override {
    return warmBootHelper_.get();
  }

  PlatformPort* getPlatformPort(PortID portID) const override;

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

  void initImpl() override {}
  virtual std::unique_ptr<BcmTestPort> createTestPort(PortID portID) const = 0;

  std::map<PortID, std::unique_ptr<BcmTestPort>> ports_;
  std::unique_ptr<BcmWarmBootHelper> warmBootHelper_;
};

} // namespace fboss
} // namespace facebook
