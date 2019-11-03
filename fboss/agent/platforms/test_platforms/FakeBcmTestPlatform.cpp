/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/test_platforms/FakeBcmTestPlatform.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/agent/platforms/test_platforms/FakeBcmTestPort.h"

#include "fboss/agent/hw/switch_asics/FakeAsic.h"

#include <folly/FileUtil.h>
#include <folly/experimental/TestUtil.h>

namespace {
static const std::array<int, 8> kMasterLogicalPortIds =
    {1, 5, 9, 13, 17, 21, 25, 29};
constexpr uint8_t kNumPortsPerTransceiver = 4;
std::unique_ptr<facebook::fboss::PlatformProductInfo> fakeProductInfo() {
  // Dummy Fruid for fake platform
  const std::string kFakeFruidJson = R"<json>({"Information": {
      "PCB Manufacturer" : "Facebook",
      "System Assembly Part Number" : "42",
      "ODM PCBA Serial Number" : "SN",
      "Product Name" : "fake_wedge",
      "Location on Fabric" : "",
      "ODM PCBA Part Number" : "PN",
      "CRC8" : "0xcc",
      "Version" : "1",
      "Product Asset Tag" : "42",
      "Product Part Number" : "42",
      "Assembled At" : "Facebook",
      "System Manufacturer" : "Facebook",
      "Product Production State" : "42",
      "Facebook PCB Part Number" : "42",
      "Product Serial Number" : "SN",
      "Local MAC" : "42:42:42:42:42:42",
      "Extended MAC Address Size" : "1",
      "Extended MAC Base" : "42:42:42:42:42:42",
      "System Manufacturing Date" : "01-01-01",
      "Product Version" : "42",
      "Product Sub-Version" : "22",
      "Facebook PCBA Part Number" : "42"
    }, "Actions": [], "Resources": []})<json>";

  folly::test::TemporaryDirectory tmpDir;
  auto fruidFilename = tmpDir.path().string() + "fruid.json";
  folly::writeFile(kFakeFruidJson, fruidFilename.c_str());
  auto productInfo =
      std::make_unique<facebook::fboss::PlatformProductInfo>(fruidFilename);
  productInfo->initialize();
  return productInfo;
}
} // namespace

namespace facebook {
namespace fboss {
FakeBcmTestPlatform::FakeBcmTestPlatform()
    : BcmTestPlatform(
          fakeProductInfo(),
          std::vector<PortID>(
              kMasterLogicalPortIds.begin(),
              kMasterLogicalPortIds.end()),
          kNumPortsPerTransceiver) {
  asic_ = std::make_unique<FakeAsic>();
}

FakeBcmTestPlatform::~FakeBcmTestPlatform() {}

std::unique_ptr<BcmTestPort> FakeBcmTestPlatform::createTestPort(PortID id) {
  return std::make_unique<FakeBcmTestPort>(id, this);
}

std::string FakeBcmTestPlatform::getVolatileStateDir() const {
  return tmpDir_.path().string() + "/volatile";
}

std::string FakeBcmTestPlatform::getPersistentStateDir() const {
  return tmpDir_.path().string() + "/persist";
}

HwAsic* FakeBcmTestPlatform::getAsic() const {
  return asic_.get();
}

} // namespace fboss
} // namespace facebook
