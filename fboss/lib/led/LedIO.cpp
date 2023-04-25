/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/led/LedIO.h"
#include <folly/Format.h>
#include <folly/logging/xlog.h>
#include <fstream>
#include <string>
#include "fboss/agent/FbossError.h"
#include "fboss/lib/led/gen-cpp2/led_mapping_types.h"

namespace {
constexpr auto kLedOn = "1";
constexpr auto kLedOff = "0";
} // namespace

namespace facebook::fboss {

LedIO::LedIO(LedMapping ledMapping) {
  id_ = *ledMapping.id();
  if (auto blue = ledMapping.bluePath()) {
    bluePath_ = *blue;
  }
  if (auto yellow = ledMapping.yellowPath()) {
    yellowPath_ = *yellow;
  }
  if (!bluePath_.has_value() && !yellowPath_.has_value()) {
    throw LedIOError(
        fmt::format("No color path set for ID {:d} (0 base)", id_));
  }
  init();
}

void LedIO::setColor(led::LedColor color) {
  if (color == currColor_) {
    return;
  }

  switch (color) {
    case led::LedColor::BLUE:
      currColor_ = led::LedColor::BLUE;
      blueOn();
      break;
    case led::LedColor::YELLOW:
      currColor_ = led::LedColor::YELLOW;
      yellowOn();
      break;
    case led::LedColor::OFF:
      if (led::LedColor::BLUE == currColor_) {
        blueOff();
      } else if (led::LedColor::YELLOW == currColor_) {
        yellowOff();
      }

      currColor_ = led::LedColor::OFF;
      XLOG(INFO) << fmt::format("Trace: set LED {:d} (0 base) to OFF", id_);
      break;
    default:
      throw LedIOError(
          fmt::format("setColor() invalid color for ID {:d} (0 base)", id_));
  }
}

led::LedColor LedIO::getColor() const {
  return currColor_;
}

void LedIO::init() {
  currColor_ = led::LedColor::OFF;
  if (bluePath_.has_value()) {
    blueOff();
  }
  if (yellowPath_.has_value()) {
    yellowOff();
  }
}

void LedIO::blueOn() {
  CHECK(bluePath_.has_value());
  setLed(*bluePath_, kLedOn);
  XLOG(INFO) << fmt::format("Trace: set LED {:d} (0 base) to Blue", id_);
}

void LedIO::blueOff() {
  CHECK(bluePath_.has_value());
  setLed(*bluePath_, kLedOff);
}

void LedIO::yellowOn() {
  CHECK(yellowPath_.has_value());
  setLed(*yellowPath_, kLedOn);
  XLOG(INFO) << fmt::format("Trace: set LED {:d} (0 base) to Yellow", id_);
}

void LedIO::yellowOff() {
  CHECK(yellowPath_.has_value());
  setLed(*yellowPath_, kLedOff);
}

void LedIO::setLed(const std::string& ledPath, const std::string& ledOp) {
  std::fstream fs;
  fs.open(ledPath, std::fstream::out);

  if (fs.is_open()) {
    fs << ledOp;
    fs.close();
  } else {
    throw LedIOError(fmt::format(
        "setLed() failed to open {} for ID {:d} (0 base)", ledPath, id_));
  }
}

} // namespace facebook::fboss
