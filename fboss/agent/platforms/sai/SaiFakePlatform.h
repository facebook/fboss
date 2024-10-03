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

#include <folly/testing/TestUtil.h>

#include "fboss/agent/platforms/sai/SaiPlatform.h"
namespace facebook::fboss {
class FakeAsic;
class SaiFakePlatform : public SaiPlatform {
 public:
  explicit SaiFakePlatform(std::unique_ptr<PlatformProductInfo> productInfo);
  ~SaiFakePlatform() override;
  std::string getHwConfig() override;
  HwAsic* getAsic() const override;
  std::vector<PortID> getAllPortsInGroup(PortID portID) const override {
    return {portID};
  }
  std::vector<FlexPortMode> getSupportedFlexPortModes() const override {
    return {
        FlexPortMode::ONEX100G,
        FlexPortMode::TWOX50G,
        FlexPortMode::ONEX40G,
        FlexPortMode::FOURX25G,
        FlexPortMode::FOURX10G};
  }

  std::optional<sai_port_interface_type_t> getInterfaceType(
      TransmitterTechnology /*transmitterTech*/,
      cfg::PortSpeed /*speed*/) const override {
    return std::nullopt;
  }

  bool isSerdesApiSupported() const override {
    return true;
  }

  bool supportInterfaceType() const override {
    // TODO: add support in fake
    return false;
  }

  void initLEDs() override {}

  const std::set<sai_api_t>& getSupportedApiList() const override;

  const AgentDirectoryUtil* getDirectoryUtil() const override {
    return agentDirUtil_.get();
  }

 private:
  void setupAsic(
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId,
      int16_t switchIndex,
      std::optional<cfg::Range64> systemPortRange,
      folly::MacAddress& mac,
      std::optional<HwAsic::FabricNodeRole> fabricNodeRole) override;
  folly::test::TemporaryDirectory tmpDir_;
  std::unique_ptr<FakeAsic> asic_;
  std::unique_ptr<AgentDirectoryUtil> agentDirUtil_;
};

} // namespace facebook::fboss
