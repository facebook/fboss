// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/fpga/HwMemoryRegion.h"

namespace facebook::fboss {

class MinipackPimController {
 public:
  enum class PimType : uint32_t {
    MINIPACK_16Q = 0xA3000000,
    MINIPACK_16O = 0xA5000000,
  };

  explicit MinipackPimController(std::unique_ptr<FpgaMemoryRegion> io)
      : io_(std::move(io)) {}

  PimType getPimType() const;

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
