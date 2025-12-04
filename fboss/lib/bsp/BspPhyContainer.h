// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
#include <folly/logging/xlog.h>
#include <memory>
#include <unordered_map>
#include "fboss/lib/bsp/BspPhyIO.h"
#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"
#include "fboss/mdio/BspDeviceMdio.h"

namespace facebook {
namespace fboss {

class BspPhyContainer {
 public:
  BspPhyContainer(int pimID, BspPhyMapping& phyMapping, BspPhyIO* phyIO);
  BspDeviceMdioController* getMdioController() const {
    return phyIO_->getMdioController();
  }
  BspPhyIO* getPhyIO() const {
    return phyIO_;
  }

 private:
  BspPhyMapping phyMapping_;
  int phyID_;
  int pimID_;
  BspPhyIO* phyIO_;
};

} // namespace fboss
} // namespace facebook
