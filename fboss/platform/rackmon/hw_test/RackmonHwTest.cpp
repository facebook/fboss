/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/rackmon/hw_test/RackmonHwTest.h"
#include <chrono>
#include <thread>

#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include "fboss/platform/helpers/Init.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"

#ifndef IS_OSS
#include "common/services/cpp/ServiceFrameworkLight.h"
#endif
#include "fboss/platform/rackmon/RackmonThriftHandler.h"

namespace rackmonsvc {

RackmonThriftService::RackmonThriftService()
    : thriftHandler_(std::make_shared<rackmonsvc::ThriftHandler>()) {
  using namespace std::chrono_literals;
  using namespace facebook::fboss::platform;

  server_ = std::make_unique<apache::thrift::ScopedServerInterfaceThread>(
      thriftHandler_, folly::SocketAddress("::1", 5973));
  // We need to have discovered at least one device in 20s.
  std::this_thread::sleep_for(20s);
}

void RackmonHwTest::SetUp() {
  folly::SocketAddress addr("::1", 5973);
  auto socket = folly::AsyncSocket::newSocket(
      folly::EventBaseManager::get()->getEventBase(), addr, 5000);
  auto channel =
      apache::thrift::RocketClientChannel::newChannel(std::move(socket));
  client_ =
      std::make_unique<apache::thrift::Client<RackmonCtrl>>(std::move(channel));
}

TEST_F(RackmonHwTest, deviceDiscovery) {
  std::vector<::rackmonsvc::ModbusDeviceInfo> devices{};
  client_->sync_listModbusDevices(devices);
  ASSERT_GT(devices.size(), 0);
  EXPECT_EQ(devices[0].mode(), ModbusDeviceMode::ACTIVE);
  EXPECT_EQ(devices[0].baudrate(), 19200);
  EXPECT_LT(devices[0].timeouts(), 5);
  EXPECT_LT(devices[0].crcErrors(), 5);
  EXPECT_LT(devices[0].miscErrors(), 5);
  EXPECT_EQ(devices[0].deviceType(), ModbusDeviceType::ORV2_PSU);
  EXPECT_GE(devices[0].devAddress(), 160);
  EXPECT_LE(devices[0].devAddress(), 191);
}

TEST_F(RackmonHwTest, value) {
  using namespace std::chrono_literals;

  client_->sync_controlRackmond(RackmonControlRequest::PAUSE_RACKMOND);
  client_->sync_controlRackmond(RackmonControlRequest::RESUME_RACKMOND);
  std::this_thread::sleep_for(5s);
  std::vector<::rackmonsvc::RackmonMonitorData> data;
  client_->sync_getMonitorData(data);
  ASSERT_GT(data.size(), 0);
  EXPECT_EQ(data[0].devInfo()->mode(), ModbusDeviceMode::ACTIVE);
  EXPECT_EQ(data[0].devInfo()->baudrate(), 19200);
  EXPECT_LT(data[0].devInfo()->timeouts(), 5);
  EXPECT_EQ(data[0].devInfo()->deviceType(), ModbusDeviceType::ORV2_PSU);
}

TEST_F(RackmonHwTest, Read) {
  ::rackmonsvc::ReadWordRegistersRequest req;
  ::rackmonsvc::ReadWordRegistersResponse resp;

  req.devAddress() = 164;
  req.regAddress() = 0;
  req.numRegisters() = 1;
  client_->sync_readHoldingRegisters(resp, req);
  ASSERT_EQ(resp.status(), ::rackmonsvc::RackmonStatusCode::SUCCESS);
}

} // namespace rackmonsvc
