/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/fpga/MinipackLed.h"

#include <folly/Format.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/FbossError.h"

namespace {
using facebook::fboss::MinipackLed;
const std::string getColorStr(MinipackLed::Color color) {
  switch (color) {
    case MinipackLed::Color::OFF:
      return "OFF";
    case MinipackLed::Color::WHITE:
      return "WHITE";
    case MinipackLed::Color::CYAN:
      return "CYAN";
    case MinipackLed::Color::BLUE:
      return "BLUE";
    case MinipackLed::Color::PINK:
      return "PINK";
    case MinipackLed::Color::RED:
      return "RED";
    case MinipackLed::Color::ORANGE:
      return "ORANGE";
    case MinipackLed::Color::YELLOW:
      return "YELLOW";
    case MinipackLed::Color::GREEN:
      return "GREEN";
    default:
      return "Unknown";
  }
}
} // unnamed namespace

namespace facebook::fboss {

void MinipackLed::setColor(MinipackLed::Color color) {
  io_->writeRegister(static_cast<uint32_t>(color));
  XLOG(DBG5) << folly::format(
      "Fpga set LED {:s} to {:s}. Register={:#x}, new value={:#x}",
      io_->getName(),
      getColorStr(color),
      io_->getBaseAddress(),
      static_cast<uint32_t>(color));
}

MinipackLed::Color MinipackLed::getColor() {
  auto color = io_->readRegister();
  switch (color) {
    case static_cast<uint32_t>(MinipackLed::Color::OFF):
    case static_cast<uint32_t>(MinipackLed::Color::WHITE):
    case static_cast<uint32_t>(MinipackLed::Color::CYAN):
    case static_cast<uint32_t>(MinipackLed::Color::BLUE):
    case static_cast<uint32_t>(MinipackLed::Color::PINK):
    case static_cast<uint32_t>(MinipackLed::Color::RED):
    case static_cast<uint32_t>(MinipackLed::Color::ORANGE):
    case static_cast<uint32_t>(MinipackLed::Color::YELLOW):
    case static_cast<uint32_t>(MinipackLed::Color::GREEN):
      auto minipackColor = static_cast<MinipackLed::Color>(color);
      XLOG(DBG5) << folly::format(
          "Fpga get LED {:s} is {:s}. Register={:#x}",
          io_->getName(),
          getColorStr(minipackColor),
          io_->getBaseAddress());
      return minipackColor;
  }
  throw FbossError("Invalid color register for MinipackLed ", color);
}

} // namespace facebook::fboss
