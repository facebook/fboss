/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/SystemPortApi.h"
#include "fboss/agent/hw/sai/api/SaiObjectApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#include <vector>

using namespace facebook::fboss;

class SystemPortApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    systemPortApi = std::make_unique<SystemPortApi>();
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<SystemPortApi> systemPortApi;
  SystemPortSaiId
  createPort(uint32_t portId, uint32_t speed = 100000, bool enabled = true) {
    sai_system_port_config_t config{
        portId, // port_id
        0, // switch_id
        10, // attached_core_index
        0, // attached_core_port_index
        speed, // speed - 100G in mbps
    };
    SaiSystemPortTraits::Attributes::ConfigInfo confInfo{config};
    SaiSystemPortTraits::Attributes::AdminState admin{enabled};
    SaiSystemPortTraits::CreateAttributes sysPort{
        confInfo, admin, std::nullopt};
    return systemPortApi->create<SaiSystemPortTraits>(sysPort, 0);
  }
};

TEST_F(SystemPortApiTest, onePort) {
  auto id = createPort(1, 100000, true);
  EXPECT_EQ(
      systemPortApi->getAttribute(
          id, SaiSystemPortTraits::Attributes::AdminState{}),
      true);
  EXPECT_EQ(
      systemPortApi->getAttribute(
          id, SaiSystemPortTraits::Attributes::QosNumberOfVoqs{}),
      8);
}
