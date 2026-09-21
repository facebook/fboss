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

#include "fboss/lib/fpga/HwMemoryRegister.h"

namespace facebook::fboss {

class MinipackLed {
 public:
  /*
  Led register map:
    [31:5]  reserved
    [4:2]   led color profile:
            (0-7) -> (white, cyan, blue, pink, red, orange, yellow, green)
    [1:1]   led flash off/on
    [0:0]   led off/on
  */
  enum class Color : uint32_t {
    OFF = 0x0,
    WHITE = 0x01,
    CYAN = 0x05,
    BLUE = 0x09,
    PINK = 0x0D,
    RED = 0x11,
    ORANGE = 0x15,
    YELLOW = 0x19,
    GREEN = 0x1D,
  };

  explicit MinipackLed(std::unique_ptr<FpgaMemoryRegister> io)
      : io_(std::move(io)) {}

  void setColor(Color color);

  Color getColor();

 private:
  std::unique_ptr<FpgaMemoryRegister> io_;
};

} // namespace facebook::fboss
