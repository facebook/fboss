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

namespace facebook::fboss::platform::rackmonsvc {

class ThriftHandler : virtual public RackmonCtrlSvIf {
 public:
  ThriftHandler() {}

  void listModbusDevices(
      std::vector<rackmonsvc::ModbusDeviceInfo>& /* devices */) override {
    // TODO
  }

  void getMonitorData(
      std::vector<rackmonsvc::RackmonMonitorData>& /* data */) override {
    // TODO
  }

  void readHoldingRegisters(
      rackmonsvc::ReadWordRegistersResponse& /* response */,
      std::unique_ptr<rackmonsvc::ReadWordRegistersRequest> /* request */)
      override {
    // TODO
  }

  rackmonsvc::RackmonStatusCode writeSingleRegister(
      std::unique_ptr<rackmonsvc::WriteSingleRegisterRequest> /* request */)
      override {
    // TODO
    return rackmonsvc::RackmonStatusCode::SUCCESS;
  }

  void getPowerLossSiren(rackmonsvc::PowerLossSiren& /* plsStatus */) override {
    // TODO
  }

  rackmonsvc::RackmonStatusCode controlRackmond(
      rackmonsvc::RackmonControlRequest /* request */) override {
    // TODO
    return rackmonsvc::RackmonStatusCode::SUCCESS;
  }
};
} // namespace facebook::fboss::platform::rackmonsvc
