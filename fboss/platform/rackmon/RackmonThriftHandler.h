/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "common/fb303/cpp/FacebookBase2.h"
#include "fboss/platform/rackmon/if/gen-cpp2/RackmonCtrl.h"

#include <memory>
#include <string>
#include <vector>

namespace facebook::fboss::platform {
class ThriftHandler : virtual public rackmon::RackmonCtrlSvIf,
                      public fb303::FacebookBase2 {
 public:
  void getPsuDevices(std::vector<rackmon::PsuDevice>& /*psuDevices*/) override {
    // TODO
  }
  void rawModbusCmd(
      rackmon::ModbusResponse& /*_return*/,
      std::unique_ptr<rackmon::ModbusRequest> /*req*/) override {
    // TODO
  }

  void configRackmond(
      rackmon::RackmonResponse& /*_return*/,
      rackmon::RackmonRequestType /*req*/) override {
    // TODO
  }

  void getPlsStatus(std::map<std::string, int32_t>& /*_return*/) override {
    // TODO
  }
};
} // namespace facebook::fboss::platform
