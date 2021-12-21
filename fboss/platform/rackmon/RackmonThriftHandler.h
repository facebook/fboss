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

#include "fboss/platform/rackmon/if/gen-cpp2/RackmonCtrl.h"

#include <memory>
#include <string>
#include <vector>

namespace facebook::fboss::platform::rackmon {

class ThriftHandler : virtual public RackmonCtrlSvIf {
 public:
  ThriftHandler() {}

  void listModbusDevices(
      std::vector<rackmon::ModbusDeviceInfo>& /* devices */) override {
    // TODO
  }

  void getMonitorData(
      std::vector<rackmon::RackmonMonitorData>& /* data */) override {
    // TODO
  }

  void readHoldingRegisters(
      rackmon::ReadWordRegistersResponse& /* response */,
      std::unique_ptr<rackmon::ReadWordRegistersRequest> /* request */)
      override {
    // TODO
  }

  rackmon::RackmonStatusCode writeSingleRegister(
      std::unique_ptr<rackmon::WriteSingleRegisterRequest> /* request */)
      override {
    // TODO
    return rackmon::RackmonStatusCode::SUCCESS;
  }

  void getPowerLossSiren(rackmon::PowerLossSiren& /* plsStatus */) override {
    // TODO
  }

  rackmon::RackmonStatusCode controlRackmond(
      rackmon::RackmonControlRequest /* request */) override {
    // TODO
    return rackmon::RackmonStatusCode::SUCCESS;
  }
};
} // namespace facebook::fboss::platform::rackmon
