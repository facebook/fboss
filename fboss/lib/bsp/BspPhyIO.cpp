// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspPhyIO.h"

#include <folly/Format.h>
#include <folly/Range.h>
#include <folly/logging/xlog.h>
#include <stdint.h>
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"
#include "fboss/mdio/BspDeviceMdio.h"

namespace facebook::fboss {

BspPhyIO::BspPhyIO(int pimID, BspPhyIOControllerInfo& controllerInfo)
    : pimID_(pimID) {
  auto mdioDevName = *controllerInfo.devicePath();
  auto controllerId = *controllerInfo.controllerId();
  mdioController_ = std::make_unique<BspDeviceMdioController>(
      controllerId, pimID, controllerId, mdioDevName);
  controllerInfo_ = controllerInfo;
  XLOG(DBG5) << fmt::format(
      "BspPhyIOTrace: BspPhyIO() successfully opened mdio {:d}, {}",
      controllerId,
      mdioDevName);
}

} // namespace facebook::fboss
