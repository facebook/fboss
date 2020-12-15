// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/fpga/FbFpgaI2c.h"
#include "fboss/lib/fpga/FbFpgaPimQsfpController.h"
#include "fboss/lib/fpga/FpgaDevice.h"

namespace facebook::fboss {

class MinipackBasePimContainer {
 public:
  // To avoid ambiguity, we explicitly decided the pim number starts from 2.
  MinipackBasePimContainer(
      int pim,
      const std::string& name,
      FpgaDevice* device,
      uint32_t pimStart,
      uint32_t pimSize,
      unsigned int portsPerPim);

  virtual ~MinipackBasePimContainer() {}

  FbFpgaPimQsfpController* getPimQsfpController();

  virtual FbFpgaI2cController* getI2cController(unsigned int port) = 0;

 protected:
  std::unique_ptr<FbFpgaPimQsfpController> pimQsfpController_;
  int pim_;
};

} // namespace facebook::fboss
