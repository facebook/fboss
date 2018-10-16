/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#include <vector>

using namespace facebook::fboss;

class PortApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    portApi = std::make_unique<PortApi>();
  }

  sai_object_id_t createPort(uint32_t speed, bool adminState) const {
    PortTypes::AttributeType asa =
        PortTypes::Attributes::AdminState(adminState);
    PortTypes::AttributeType sa = PortTypes::Attributes::Speed(speed);
    return portApi->create({asa, sa}, 0);
  }

  std::vector<sai_object_id_t> createFourPorts() const {
    std::vector<sai_object_id_t> portIds;
    portIds.push_back(createPort(100000, true));
    for (int i = 0; i < 3; ++i) {
      portIds.push_back(createPort(25000, false));
    }
    for (const auto& portId : portIds) {
      checkPort(portId);
    }
    return portIds;
  }

  void checkPort(sai_object_id_t portId) const {
    PortTypes::Attributes::AdminState asa;
    PortTypes::Attributes::Speed sa;
    auto gotAdminState = portApi->getAttribute(asa, portId);
    auto gotSpeed = portApi->getAttribute(sa, portId);
    EXPECT_EQ(fs->pm.get(portId).adminState, gotAdminState);
    EXPECT_EQ(fs->pm.get(portId).speed, gotSpeed);
    EXPECT_EQ(fs->pm.get(portId).id, portId);
  }

  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<PortApi> portApi;
};

TEST_F(PortApiTest, onePort) {
  auto id = createPort(100000, true);
  PortTypes::Attributes::AdminState as_blank;
  PortTypes::Attributes::Speed s_blank;
  EXPECT_EQ(portApi->getAttribute(as_blank, id), true);
  EXPECT_EQ(portApi->getAttribute(s_blank, id), 100000);
}

TEST_F(PortApiTest, fourPorts) {
  auto portIds = createFourPorts();
}

TEST_F(PortApiTest, setPortAttributes) {
  auto portIds = createFourPorts();

  PortTypes::Attributes::AdminState as_attr(true);
  PortTypes::Attributes::Speed speed_attr(50000);
  // set speeds
  portApi->setAttribute(speed_attr, portIds[0]);
  portApi->setAttribute(speed_attr, portIds[2]);
  // set admin state
  portApi->setAttribute(as_attr, portIds[2]);
  // confirm admin states
  EXPECT_EQ(portApi->getAttribute(as_attr, portIds[0]), true);
  EXPECT_EQ(portApi->getAttribute(as_attr, portIds[1]), false);
  EXPECT_EQ(portApi->getAttribute(as_attr, portIds[2]), true);
  EXPECT_EQ(portApi->getAttribute(as_attr, portIds[3]), false);
  // confirm speeds
  EXPECT_EQ(portApi->getAttribute(speed_attr, portIds[0]), 50000);
  EXPECT_EQ(portApi->getAttribute(speed_attr, portIds[1]), 25000);
  EXPECT_EQ(portApi->getAttribute(speed_attr, portIds[2]), 50000);
  EXPECT_EQ(portApi->getAttribute(speed_attr, portIds[3]), 25000);
  // confirm consistency internally, too
  for (const auto& portId : portIds) {
    checkPort(portId);
  }
}

TEST_F(PortApiTest, removePort) {
  {
    // basic remove
    auto portId = createPort(25000, true);
    portApi->remove(portId);
  }
  {
    // remove a const portId
    const auto portId = createPort(25000, true);
    portApi->remove(portId);
  }
  {
    // remove rvalue id
    portApi->remove(createPort(25000, true));
  }
  {
    // remove in "canonical" for-loop
    auto portIds = createFourPorts();
    for (const auto& portId : portIds) {
      portApi->remove(portId);
    }
  }
}
