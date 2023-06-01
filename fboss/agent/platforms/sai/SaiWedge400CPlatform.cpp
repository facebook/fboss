/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiWedge400CPlatform.h"
#include "fboss/agent/hw/switch_asics/EbroAsic.h"
#include "fboss/agent/platforms/common/ebb_lab/Wedge400CEbbLabPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400c/Wedge400CFabricPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400c/Wedge400CGrandTetonPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400c/Wedge400CPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400c/Wedge400CPlatformUtil.h"
#include "fboss/agent/platforms/common/wedge400c/Wedge400CVoqPlatformMapping.h"
#include "fboss/agent/platforms/sai/SaiWedge400CPlatformPort.h"

#include <algorithm>

namespace facebook::fboss {

SaiWedge400CPlatform::SaiWedge400CPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiTajoPlatform(
          std::move(productInfo),
          createWedge400CPlatformMapping(platformMappingStr),
          localMac) {}

SaiWedge400CPlatform::SaiWedge400CPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<Wedge400CEbbLabPlatformMapping> mapping,
    folly::MacAddress localMac)
    : SaiTajoPlatform(std::move(productInfo), std::move(mapping), localMac) {}

SaiWedge400CPlatform::SaiWedge400CPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<Wedge400CVoqPlatformMapping> mapping,
    folly::MacAddress localMac)
    : SaiTajoPlatform(std::move(productInfo), std::move(mapping), localMac) {}

SaiWedge400CPlatform::SaiWedge400CPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<Wedge400CFabricPlatformMapping> mapping,
    folly::MacAddress localMac)
    : SaiTajoPlatform(std::move(productInfo), std::move(mapping), localMac) {}

void SaiWedge400CPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    std::optional<cfg::Range64> systemPortRange,
    folly::MacAddress& mac) {
  asic_ =
      std::make_unique<EbroAsic>(switchType, switchId, systemPortRange, mac);
#if defined(TAJO_SDK_VERSION_1_62_0)
  asic_->setDefaultStreamType(cfg::StreamType::UNICAST);
#endif
}

HwAsic* SaiWedge400CPlatform::getAsic() const {
  return asic_.get();
}

std::string SaiWedge400CPlatform::getHwConfig() {
  return *config()->thrift.platform()->get_chip().get_asic().config();
}

std::vector<sai_system_port_config_t>
SaiWedge400CPlatform::getInternalSystemPortConfig() const {
  CHECK(asic_) << " Asic must be set before getting sys port info";
  CHECK(asic_->getSwitchId()) << " Switch Id must be set before sys port info";
  auto switchIdVal = static_cast<uint32_t>(*asic_->getSwitchId());
  // Below are a mixture system port configs for
  // internal {loopback, CPU} ports. From the speeds (in Mbps)
  // one can infer that ports 6-10 are 1G CPU ports and 10-14 are
  // 100G loopback ports (TODO - confirm this with vendor)
  //
  return {
      {11, switchIdVal, 0, 25, 100000, 8},
      {12, switchIdVal, 2, 25, 100000, 8},
      {13, switchIdVal, 4, 25, 100000, 8},
      {14, switchIdVal, 6, 25, 100000, 8},
      {15, switchIdVal, 8, 25, 100000, 8},
      {16, switchIdVal, 10, 25, 100000, 8},
      {6, switchIdVal, 0, 24, 1000, 8},
      {7, switchIdVal, 4, 24, 1000, 8},
      {8, switchIdVal, 6, 24, 1000, 8},
      {9, switchIdVal, 8, 24, 1000, 8},
      {10, switchIdVal, 1, 24, 1000, 8}};
}

SaiWedge400CPlatform::~SaiWedge400CPlatform() {}

SaiWedge400CEbbLabPlatform::SaiWedge400CEbbLabPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiWedge400CPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<Wedge400CEbbLabPlatformMapping>()
              : std::make_unique<Wedge400CEbbLabPlatformMapping>(
                    platformMappingStr),
          localMac) {}

std::unique_ptr<PlatformMapping>
SaiWedge400CPlatform::createWedge400CPlatformMapping(
    const std::string& platformMappingStr) {
  if (utility::isWedge400CPlatformRackTypeGrandTeton()) {
    return platformMappingStr.empty()
        ? std::make_unique<Wedge400CGrandTetonPlatformMapping>()
        : std::make_unique<Wedge400CGrandTetonPlatformMapping>(
              platformMappingStr);
  }
  return platformMappingStr.empty()
      ? std::make_unique<Wedge400CPlatformMapping>()
      : std::make_unique<Wedge400CPlatformMapping>(platformMappingStr);
}

SaiWedge400CVoqPlatform::SaiWedge400CVoqPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiWedge400CPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<Wedge400CVoqPlatformMapping>()
              : std::make_unique<Wedge400CVoqPlatformMapping>(
                    platformMappingStr),
          localMac) {}

SaiWedge400CFabricPlatform::SaiWedge400CFabricPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiWedge400CPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<Wedge400CFabricPlatformMapping>()
              : std::make_unique<Wedge400CFabricPlatformMapping>(
                    platformMappingStr),
          localMac) {}

} // namespace facebook::fboss
