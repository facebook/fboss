/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/tests/utils/FakeBcmTestPlatform.h"

#include "fboss/agent/hw/bcm/BcmCosQueueManagerUtils.h"
#include "fboss/agent/hw/bcm/BcmPortIngressBufferManager.h"
#include "fboss/agent/hw/switch_asics/FakeAsic.h"
#include "fboss/agent/platforms/common/fake_test/FakeTestPlatformMapping.h"
#include "fboss/agent/platforms/tests/utils/FakeBcmTestPort.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

namespace {
std::vector<int> getControllingPortIDs() {
  std::vector<int> results;
  for (int i = 1; i < 33; i += 4) {
    results.push_back(i);
  }
  return results;
}
} // namespace

// @lint-ignore CLANGTIDY
DECLARE_string(mac);

namespace facebook::fboss {

FakeBcmTestPlatform::FakeBcmTestPlatform()
    : BcmTestPlatform(
          fakeProductInfo(),
          std::make_unique<FakeTestPlatformMapping>(getControllingPortIDs())) {
  agentDirUtil_ = std::make_unique<AgentDirectoryUtil>(
      tmpDir_.path().string() + "/volatile",
      tmpDir_.path().string() + "/persist");
  FLAGS_mac = "02:00:00:00:00:01";
}

void FakeBcmTestPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    int16_t switchIndex,
    std::optional<cfg::Range64> systemPortRange,
    folly::MacAddress& mac,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<FakeAsic>(
      switchType, switchId, switchIndex, systemPortRange, mac);
}

FakeBcmTestPlatform::~FakeBcmTestPlatform() {}

std::unique_ptr<BcmTestPort> FakeBcmTestPlatform::createTestPort(PortID id) {
  return std::make_unique<FakeBcmTestPort>(id, this);
}

HwAsic* FakeBcmTestPlatform::getAsic() const {
  return asic_.get();
}

const PortQueue& FakeBcmTestPlatform::getDefaultPortQueueSettings(
    cfg::StreamType streamType) const {
  return utility::getDefaultPortQueueSettings(
      utility::BcmChip::TOMAHAWK, streamType);
}

const PortQueue& FakeBcmTestPlatform::getDefaultControlPlaneQueueSettings(
    cfg::StreamType streamType) const {
  return utility::getDefaultControlPlaneQueueSettings(
      utility::BcmChip::TOMAHAWK, streamType);
}

const PortPgConfig& FakeBcmTestPlatform::getDefaultPortPgSettings() const {
  return BcmPortIngressBufferManager::getDefaultChipPgSettings(
      utility::BcmChip::TOMAHAWK3);
}

const BufferPoolCfg& FakeBcmTestPlatform::getDefaultPortIngressPoolSettings()
    const {
  return BcmPortIngressBufferManager::getDefaultChipIngressPoolSettings(
      utility::BcmChip::TOMAHAWK3);
}

} // namespace facebook::fboss
