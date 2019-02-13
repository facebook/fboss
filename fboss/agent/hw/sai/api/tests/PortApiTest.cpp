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

  sai_object_id_t createPort(
      uint32_t speed,
      const std::vector<uint32_t>& lanes,
      bool adminState) const {
    PortApiParameters::Attributes a{{lanes, speed, adminState}};
    return portApi->create2(a.attrs(), 0);
  }

  std::vector<sai_object_id_t> createFourPorts() const {
    std::vector<sai_object_id_t> portIds;
    portIds.push_back(createPort(100000, {0, 1, 2, 3}, true));
    for (uint32_t i = 4; i < 8; ++i) {
      portIds.push_back(createPort(25000, {i}, false));
    }
    for (const auto& portId : portIds) {
      checkPort(portId);
    }
    return portIds;
  }

  void checkPort(sai_object_id_t portId) const {
    PortApiParameters::Attributes::AdminState adminStateAttribute;
    std::vector<uint32_t> ls;
    ls.resize(4);
    PortApiParameters::Attributes::HwLaneList hwLaneListAttribute(ls);
    PortApiParameters::Attributes::Speed speedAttribute;
    auto gotAdminState = portApi->getAttribute(adminStateAttribute, portId);
    auto gotSpeed = portApi->getAttribute(speedAttribute, portId);
    auto lanes = portApi->getAttribute(hwLaneListAttribute, portId);
    EXPECT_EQ(fs->pm.get(portId).adminState, gotAdminState);
    EXPECT_EQ(fs->pm.get(portId).speed, gotSpeed);
    EXPECT_EQ(fs->pm.get(portId).id, portId);
    EXPECT_EQ(fs->pm.get(portId).lanes, lanes);
  }

  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<PortApi> portApi;
};

TEST_F(PortApiTest, onePort) {
  auto id = createPort(100000, {42}, true);
  PortApiParameters::Attributes::AdminState as_blank;
  std::vector<uint32_t> ls;
  ls.resize(1);
  PortApiParameters::Attributes::HwLaneList l_blank(ls);
  PortApiParameters::Attributes::Speed s_blank;
  EXPECT_EQ(portApi->getAttribute(as_blank, id), true);
  EXPECT_EQ(portApi->getAttribute(s_blank, id), 100000);
  auto lanes = portApi->getAttribute(l_blank, id);
  EXPECT_EQ(lanes.size(), 1);
  EXPECT_EQ(lanes[0], 42);
}

TEST_F(PortApiTest, fourPorts) {
  auto portIds = createFourPorts();
}

TEST_F(PortApiTest, setPortAttributes) {
  auto portIds = createFourPorts();

  PortApiParameters::Attributes::AdminState as_attr(true);
  PortApiParameters::Attributes::Speed speed_attr(50000);
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
    auto portId = createPort(25000, {42}, true);
    portApi->remove(portId);
  }
  {
    // remove a const portId
    const auto portId = createPort(25000, {42}, true);
    portApi->remove(portId);
  }
  {
    // remove rvalue id
    portApi->remove(createPort(25000, {42}, true));
  }
  {
    // remove in "canonical" for-loop
    auto portIds = createFourPorts();
    for (const auto& portId : portIds) {
      portApi->remove(portId);
    }
  }
}
