/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmPlatform.h"

#include <folly/FileUtil.h>
#include <folly/String.h>

#include "fboss/agent/SysError.h"
#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

DEFINE_bool(
    enable_routes_in_host_table,
    false,
    "Whether to program host routes in host table. If false, all "
    "routes are programmed in route table");

namespace facebook::fboss {

std::string BcmPlatform::getHwConfigDumpFile() const {
  return getDirectoryUtil()->getVolatileStateDir() + "/" + FLAGS_hw_config_file;
}

bool BcmPlatform::isBcmShellSupported() const {
  return true;
}

bool BcmPlatform::isDisableHotSwapSupported() const {
  return true;
}

void BcmPlatform::dumpHwConfig() const {
  auto hwConfigFile = getHwConfigDumpFile();
  if (getAsic()->isSupported(HwAsic::Feature::HSDK)) {
    BcmAPI::getHwYamlConfig().dumpConfig(hwConfigFile);
  } else {
    std::vector<std::string> nameValStrs;
    for (const auto& kv : BcmAPI::getHwConfig()) {
      nameValStrs.emplace_back(
          folly::to<std::string>(kv.first, '=', kv.second));
    }
    auto bcmConf = folly::join('\n', nameValStrs);
    if (!folly::writeFile(bcmConf, hwConfigFile.c_str())) {
      throw facebook::fboss::SysError(errno, "error writing bcm config ");
    }
  }
}

uint32_t BcmPlatform::getMMUCellBytes() const {
  throw FbossError("Unsupported platform for retrieving MMU cell bytes");
}

int BcmPlatform::getPortItm(BcmPort* /*bcmPort*/) const {
  throw FbossError("Unsupported platform for retrieving ITM for port");
}

const PortPgConfig& BcmPlatform::getDefaultPortPgSettings() const {
  throw FbossError("Unsupported platform for retrieving PG settings ");
}

const BufferPoolCfg& BcmPlatform::getDefaultPortIngressPoolSettings() const {
  throw FbossError(
      "Unsupported platform for retrieving Ingress Pool settings ");
}

phy::VCOFrequency BcmPlatform::getVCOFrequency(
    phy::VCOFrequencyFactor& factor) const {
  auto speed = *factor.speed();
  auto fecMode = *factor.fecMode();
  switch (speed) {
    case cfg::PortSpeed::FOURHUNDREDG:
      return phy::VCOFrequency::VCO_26_5625GHZ;
    case cfg::PortSpeed::TWOHUNDREDG:
      [[fallthrough]];
    case cfg::PortSpeed::HUNDREDG:
      [[fallthrough]];
    case cfg::PortSpeed::FIFTYG:
      [[fallthrough]];
    case cfg::PortSpeed::FIFTYTHREEPOINTONETWOFIVEG:
      [[fallthrough]];
    case cfg::PortSpeed::HUNDREDANDSIXPOINTTWOFIVEG:
      [[fallthrough]];
    case cfg::PortSpeed::TWENTYFIVEG:
      switch (fecMode) {
        case phy::FecMode::RS544:
          [[fallthrough]];
        case phy::FecMode::RS544_2N:
          return phy::VCOFrequency::VCO_26_5625GHZ;
        case phy::FecMode::NONE:
          [[fallthrough]];
        case phy::FecMode::CL74:
          [[fallthrough]];
        case phy::FecMode::CL91:
          [[fallthrough]];
        case phy::FecMode::RS545:
          [[fallthrough]];
        case phy::FecMode::RS528:
          return phy::VCOFrequency::VCO_25_78125GHZ;
      }
    case cfg::PortSpeed::FORTYG:
      [[fallthrough]];
    case cfg::PortSpeed::TWENTYG:
      [[fallthrough]];
    case cfg::PortSpeed::XG:
      return phy::VCOFrequency::VCO_20_625GHZ;
    case cfg::PortSpeed::GIGE:
      [[fallthrough]];
    case cfg::PortSpeed::EIGHTHUNDREDG:
    case cfg::PortSpeed::DEFAULT:
      return phy::VCOFrequency::UNKNOWN;
  }
  return phy::VCOFrequency::UNKNOWN;
}
} // namespace facebook::fboss
