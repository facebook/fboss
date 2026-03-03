// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
#include <folly/logging/xlog.h>
#include <optional>
#include "fboss/lib/bsp/BspPhyIO.h"
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

  void initPhy(bool forceReset = true) const;
  bool hasPhyResetPath() const;
  std::optional<std::string> getPhyResetPath() const;

 private:
  BspPhyMapping phyMapping_;
  int phyID_;
  int pimID_;
  BspPhyIO* phyIO_;
  std::optional<std::string> phyResetPath_;
};

} // namespace fboss
} // namespace facebook
