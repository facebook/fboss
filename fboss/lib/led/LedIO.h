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
  void setColor(led::LedColor color);
  led::LedColor getColor() const;

  // Forbidden copy constructor and assignment operator
  LedIO(LedIO const&) = delete;
  LedIO& operator=(LedIO const&) = delete;

 private:
  void init();
  void blueOn();
  void blueOff();
  void yellowOn();
  void yellowOff();
  void setLed(const std::string& ledPath, const std::string& ledOp);

  led::LedColor currColor_;
  uint32_t id_;
  std::optional<std::string> bluePath_;
  std::optional<std::string> yellowPath_;
};

} // namespace facebook::fboss
