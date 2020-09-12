/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/lib/fpga/FbFpga.h"

namespace facebook::fboss {
/**
 * FPGA device in Fuji platform.
 */
class FujiFpga : public FbFpga {
 public:
  FujiFpga();
  static std::shared_ptr<FujiFpga> getInstance();

  // TODO: Fuji dom fpga led/mdio part is very similar to Minipack.
  // Thus, just use existing FbDomFpga here for now. However, we still
  // need to look into qsfp part and extend/refactor FbDomFpga class,
  // if necessary.
};

} // namespace facebook::fboss
