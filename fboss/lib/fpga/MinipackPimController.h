// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/fpga/HwMemoryRegion.h"

namespace facebook::fboss {

class MinipackPimController {
 public:
  explicit MinipackPimController(std::unique_ptr<FpgaMemoryRegion> io)
      : io_(std::move(io)) {}

  // TODO(pgardideh): this is temporary. eventually memory region ownership
  // shall be moved to separate controllers and should not be accessed from
  // outside. This is mostly needed for mdio classes
  FpgaMemoryRegion* getMemoryRegion() {
    return io_.get();
  }

 private:
  std::unique_ptr<FpgaMemoryRegion> io_;
};

} // namespace facebook::fboss
