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

#include <folly/experimental/TestUtil.h>

#include "fboss/agent/platforms/sai/SaiPlatform.h"
namespace facebook::fboss {
class FakeAsic;
class SaiFakePlatform : public SaiPlatform {
 public:
  explicit SaiFakePlatform(std::unique_ptr<PlatformProductInfo> productInfo);
  ~SaiFakePlatform() override;
  std::string getVolatileStateDir() const override;
  std::string getPersistentStateDir() const override;
  std::string getHwConfig() override;
  HwAsic* getAsic() const override;
  bool getObjectKeysSupported() const override {
    return true;
  }
  uint32_t numLanesPerCore() const override {
    return 4;
  }
  std::vector<PortID> getAllPortsInGroup(PortID portID) const override {
    return {portID};
  }
  std::vector<FlexPortMode> getSupportedFlexPortModes() const override {
    return {FlexPortMode::ONEX100G,
            FlexPortMode::TWOX50G,
            FlexPortMode::ONEX40G,
            FlexPortMode::FOURX25G,
            FlexPortMode::FOURX10G};
  }

  folly::MacAddress getLocalMac() const override;

  std::optional<sai_port_interface_type_t> getInterfaceType(
      TransmitterTechnology /*transmitterTech*/,
      cfg::PortSpeed /*speed*/) const override {
    return std::nullopt;
  }

  bool isSerdesApiSupported() override {
    // TODO: add support in fake
    return false;
  }

  bool supportInterfaceType() const override {
    // TODO: add support in fake
    return false;
  }

  void initLEDs() override {}

 private:
  folly::test::TemporaryDirectory tmpDir_;
  std::unique_ptr<FakeAsic> asic_;
};

} // namespace facebook::fboss
