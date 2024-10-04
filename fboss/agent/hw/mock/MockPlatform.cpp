/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/mock/MockPlatform.h"

#include <folly/Memory.h>
#include "fboss/agent/Platform.h"
#include "fboss/agent/SysError.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/hw/mock/MockHwSwitch.h"
#include "fboss/agent/hw/mock/MockPlatformMapping.h"
#include "fboss/agent/hw/mock/MockPlatformPort.h"
#include "fboss/agent/hw/mock/MockTestHandle.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

#include <gmock/gmock.h>
#include <memory>

using std::make_unique;
using std::string;
using std::unique_ptr;
using testing::_;
using testing::Invoke;
using testing::WithArg;

namespace facebook::fboss {

const folly::MacAddress& MockPlatform::getMockLocalMac() {
  static const folly::MacAddress kMockLocalMac("00:02:00:ab:cd:ef");
  return kMockLocalMac;
}

const folly::IPAddressV6& MockPlatform::getMockLinkLocalIp6() {
  static const folly::IPAddressV6 kMockLinkLocalIp6(
      folly::IPAddressV6::LINK_LOCAL, getMockLocalMac());
  return kMockLinkLocalIp6;
}

MockPlatform::MockPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<MockHwSwitch> hw)
    : Platform(
          std::move(productInfo),
          std::make_unique<MockPlatformMapping>(),
          getMockLocalMac()),
      tmpDir_("fboss_mock_state"),
      hw_(std::move(hw)),
      agentDirUtil_(new AgentDirectoryUtil(
          tmpDir_.path().string() + "/volatile",
          tmpDir_.path().string() + "/persist")) {
  ON_CALL(*hw_, stateChangedImpl(_))
      .WillByDefault(WithArg<0>(
          Invoke([=](const StateDelta& delta) { return delta.newState(); })));
  ON_CALL(*hw_, stateChangedTransaction(_, _))
      .WillByDefault(WithArg<0>(
          Invoke([=](const StateDelta& delta) { return delta.newState(); })));
  for (const auto& portEntry : getPlatformMapping()->getPlatformPorts()) {
    auto portID = PortID(portEntry.first);
    portMapping_.emplace(
        portID, std::make_unique<MockPlatformPort>(portID, this));
  }
}

MockPlatform::MockPlatform()
    : MockPlatform(
          nullptr,
          make_unique<::testing::NiceMock<MockHwSwitch>>(this)) {}

MockPlatform::~MockPlatform() = default;

void MockPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    int16_t switchIndex,
    std::optional<cfg::Range64> systemPortRange,
    folly::MacAddress& mac,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  asic_ = std::make_unique<MockAsic>(
      switchType, switchId, switchIndex, systemPortRange, mac);
}
HwSwitch* MockPlatform::getHwSwitch() const {
  return hw_.get();
}

HwAsic* MockPlatform::getAsic() const {
  return asic_.get();
}

PlatformPort* MockPlatform::getPlatformPort(PortID id) const {
  if (auto port = portMapping_.find(id); port != portMapping_.end()) {
    return port->second.get();
  }
  throw FbossError("Can't find MockPlatform PlatformPort for ", id);
}

HwSwitchWarmBootHelper* MockPlatform::getWarmBootHelper() {
  throw FbossError("Mock platforms don't have warmboot helper.");
}
} // namespace facebook::fboss
