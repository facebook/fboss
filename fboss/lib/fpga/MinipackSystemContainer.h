// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/fpga/MinipackBaseSystemContainer.h"

namespace facebook::fboss {

class MinipackSystemContainer : public MinipackBaseSystemContainer {
 public:
  MinipackSystemContainer();
  static std::shared_ptr<MinipackSystemContainer> getInstance();

 private:
  // Forbidden copy constructor and assignment operator
  MinipackSystemContainer(MinipackSystemContainer const&) = delete;
  MinipackSystemContainer& operator=(MinipackSystemContainer const&) = delete;
};

} // namespace facebook::fboss
