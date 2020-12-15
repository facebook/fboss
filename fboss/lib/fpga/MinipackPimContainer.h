// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/fpga/MinipackBaseSystemContainer.h"

namespace facebook::fboss {

class MinipackPimContainer : public MinipackBasePimContainer {
 public:
  MinipackPimContainer(
      int pim,
      const std::string& name,
      FpgaDevice* device,
      uint32_t start,
      uint32_t size);
  ~MinipackPimContainer() override {}

  FbFpgaI2cController* getI2cController(unsigned int port) override;

 private:
  std::vector<std::unique_ptr<FbFpgaI2cController>> i2cControllers_;
};

} // namespace facebook::fboss
