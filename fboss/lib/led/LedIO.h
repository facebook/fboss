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

#include <stdexcept>
#include "fboss/led_service/if/gen-cpp2/led_structs_types.h"
#include "fboss/lib/led/gen-cpp2/led_mapping_types.h"

namespace facebook::fboss {

constexpr auto kLedOff = "0";
constexpr auto kLedMaxBrightnessPath = "/max_brightness";
constexpr auto kLedBrightnessPath = "/brightness";
constexpr auto kLedTriggerPath = "/trigger";
constexpr auto kLedDelayOnPath = "/delay_on";
constexpr auto kLedDelayOffPath = "/delay_off";
constexpr auto kLedTimerTrigger = "timer";
constexpr auto kLedBlinkOff = "0";
constexpr auto kLedBlinkSlow = "1000";
constexpr auto kLedBlinkFast = "500";
constexpr auto kMinBrightness = 1;
constexpr auto kMaxBrightness = 255;

class LedIOError : public std::runtime_error {
 public:
  explicit LedIOError(const std::string& what) : runtime_error(what) {}
};

/* Blue = link up
 * Yellow = the expected lldp neighbour in the agent config does not match the
 * lldp packet on the wire
 * Off = everything else
 */
class LedIO {
 public:
  explicit LedIO(LedMapping ledMapping);
  ~LedIO() {}
  void setLedState(led::LedState state);
  led::LedState getLedState() const;

  // Forbidden copy constructor and assignment operator
  LedIO(LedIO const&) = delete;
  LedIO& operator=(LedIO const&) = delete;

 private:
  void init();
  void blueOn(led::Blink blink);
  void blueOff();
  void yellowOn(led::Blink blink);
  void yellowOff();
  void turnOffAllLeds();
  void setLed(const std::string& ledPath, const std::string& ledOp);
  void setBlink(const std::string& ledPath, led::Blink blink);

  // Max brightness for the LED is determined by /max_brightness
  // per LED.
  void initMaxBrightness(const std::string& path, std::string& maxBrightness);

  led::LedState currState_;
  const uint32_t id_;
  std::string bluePath_;
  std::string blueMaxBrightness_;
  std::string yellowPath_;
  std::string yellowMaxBrightness_;
};

} // namespace facebook::fboss
