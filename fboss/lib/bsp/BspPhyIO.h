// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"
#include "fboss/lib/i2c/I2cController.h"
#include "fboss/mdio/BspDeviceMdio.h"

namespace facebook {
namespace fboss {

class BspPhyIOError : public std::exception {
 public:
  explicit BspPhyIOError(const std::string& what) : what_(what) {}

  const char* what() const noexcept override {
    return what_.c_str();
  }

 private:
  std::string what_;
};

class BspPhyIO {
 public:
  BspPhyIO(int pimID, BspPhyIOControllerInfo& controllerInfo);
  ~BspPhyIO() {}
  BspDeviceMdioController* getMdioController() {
    return mdioController_.get();
  }

 private:
  std::unique_ptr<BspDeviceMdioController> mdioController_;
  BspPhyIOControllerInfo controllerInfo_;
  int phyID_;
  int pimID_;
};

} // namespace fboss
} // namespace facebook
