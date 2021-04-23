// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/fpga/MinipackBaseSystemContainer.h"

namespace facebook::fboss {

class MinipackSystemContainer : public MinipackBaseSystemContainer {
 public:
  MinipackSystemContainer();
  explicit MinipackSystemContainer(std::unique_ptr<FpgaDevice> fpgaDevice);

  static std::shared_ptr<MinipackSystemContainer> getInstance();

  MultiPimPlatformPimContainer::PimType getPimType(int pim) override;

 private:
  // Forbidden copy constructor and assignment operator
  MinipackSystemContainer(MinipackSystemContainer const&) = delete;
  MinipackSystemContainer& operator=(MinipackSystemContainer const&) = delete;
};

} // namespace facebook::fboss
