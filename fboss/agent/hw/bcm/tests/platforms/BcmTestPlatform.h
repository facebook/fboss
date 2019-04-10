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

#include "fboss/agent/types.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/tests/platforms/BcmTestPort.h"

#include <folly/MacAddress.h>

#include <map>
#include <memory>

namespace facebook {
namespace fboss {

enum class FlexPortMode {
  FOURX10G,
  FOURX25G,
  ONEX40G,
  TWOX50G,
  ONEX100G,
};

class BcmTestPlatform : public BcmPlatform {
 public:
  BcmTestPlatform(
      std::vector<int> masterLogicalPortIds,
      int numPortsPerTranceiver);
  ~BcmTestPlatform() override;

  static folly::MacAddress kLocalCpuMac();

  HwSwitch* getHwSwitch() const override;
  void onHwInitialized(SwSwitch* sw) override;
  void onInitialConfigApplied(SwSwitch* sw) override;
  std::unique_ptr<ThriftHandler> createHandler(SwSwitch* sw) override;

  folly::MacAddress getLocalMac() const override;
  virtual cfg::PortSpeed getMaxPortSpeed() = 0;
  virtual std::list<FlexPortMode> getSupportedFlexPortModes() const = 0;

  void onUnitCreate(int unit) override;
  void onUnitAttach(int unit) override;
  InitPortMap initPorts() override;

  void getProductInfo(ProductInfo& /*info*/) override{
      // Nothing to do
  };
  bool canUseHostTableForHostRoutes() const override {
    return true;
  }
  virtual bool hasLinkScanCapability() const  = 0;
  TransceiverIdxThrift getPortMapping(PortID /* unused */) const override {
    return TransceiverIdxThrift();
  }
  bool isBcmShellSupported() const override {
    return true;
  }

  const std::vector<int>& logicalPortIds() const {
    return logicalPortIds_;
  }
  const std::vector<int>& masterLogicalPortIds() const {
    return masterLogicalPortIds_;
  }
  std::vector<int> getAllPortsinGroup(int portID);

  BcmWarmBootHelper* getWarmBootHelper() override {
    return warmBootHelper_.get();
  }

  PlatformPort* getPlatformPort(PortID portID) const override;

 protected:
  // Each platform should have their own logical ports list.
  std::vector<int> logicalPortIds_;
  // List of controlling ports on platform. Each of these then
  // have subports that can be used when using flex ports
  std::vector<int> masterLogicalPortIds_;
  int numPortsPerTranceiver_;

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestPlatform(BcmTestPlatform const &) = delete;
  BcmTestPlatform& operator=(BcmTestPlatform const &) = delete;

  void initImpl() override {}
  virtual std::unique_ptr<BcmTestPort> createTestPort(PortID portID) const = 0;

  std::map<PortID, std::unique_ptr<BcmTestPort>> ports_;
  std::unique_ptr<BcmWarmBootHelper> warmBootHelper_;
};

} // namespace fboss
} // namespace facebook
