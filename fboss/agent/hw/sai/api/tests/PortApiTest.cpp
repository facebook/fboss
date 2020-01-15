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
#include "fboss/agent/hw/sai/api/SaiObjectApi.h"
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

  PortSaiId createPort(
      uint32_t speed,
      const std::vector<uint32_t>& lanes,
      bool adminState) const {
    SaiPortTraits::CreateAttributes a{lanes,
                                      speed,
                                      adminState,
                                      std::nullopt,
                                      std::nullopt,
                                      std::nullopt,
                                      std::nullopt,
                                      std::nullopt,
                                      std::nullopt};
    return portApi->create<SaiPortTraits>(a, 0);
  }

  std::vector<PortSaiId> createFivePorts() const {
    std::vector<PortSaiId> portIds;
    portIds.push_back(createPort(100000, {0, 1, 2, 3}, true));
    for (uint32_t i = 4; i < 8; ++i) {
      portIds.push_back(createPort(25000, {i}, false));
    }
    for (const auto& portId : portIds) {
      checkPort(portId);
    }
    return portIds;
  }

  void checkPort(PortSaiId portId) const {
    SaiPortTraits::Attributes::AdminState adminStateAttribute;
    std::vector<uint32_t> ls;
    ls.resize(4);
    SaiPortTraits::Attributes::HwLaneList hwLaneListAttribute(ls);
    SaiPortTraits::Attributes::Speed speedAttribute;
    auto gotAdminState = portApi->getAttribute(portId, adminStateAttribute);
    auto gotSpeed = portApi->getAttribute(portId, speedAttribute);
    auto lanes = portApi->getAttribute(portId, hwLaneListAttribute);
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
  SaiPortTraits::Attributes::AdminState as_blank;
  std::vector<uint32_t> ls;
  ls.resize(1);
  SaiPortTraits::Attributes::HwLaneList l_blank(ls);
  SaiPortTraits::Attributes::Speed s_blank;
  EXPECT_EQ(portApi->getAttribute(id, as_blank), true);
  EXPECT_EQ(portApi->getAttribute(id, s_blank), 100000);
  auto lanes = portApi->getAttribute(id, l_blank);
  EXPECT_EQ(lanes.size(), 1);
  EXPECT_EQ(lanes[0], 42);
}

TEST_F(PortApiTest, fourPorts) {
  auto portIds = createFivePorts();
}

TEST_F(PortApiTest, setPortAttributes) {
  auto portIds = createFivePorts();

  SaiPortTraits::Attributes::AdminState as_attr(true);
  SaiPortTraits::Attributes::Speed speed_attr(50000);
  // set speeds
  portApi->setAttribute(portIds[0], speed_attr);
  portApi->setAttribute(portIds[2], speed_attr);
  // set admin state
  portApi->setAttribute(portIds[2], as_attr);
  // confirm admin states
  EXPECT_EQ(portApi->getAttribute(portIds[0], as_attr), true);
  EXPECT_EQ(portApi->getAttribute(portIds[1], as_attr), false);
  EXPECT_EQ(portApi->getAttribute(portIds[2], as_attr), true);
  EXPECT_EQ(portApi->getAttribute(portIds[3], as_attr), false);
  // confirm speeds
  EXPECT_EQ(portApi->getAttribute(portIds[0], speed_attr), 50000);
  EXPECT_EQ(portApi->getAttribute(portIds[1], speed_attr), 25000);
  EXPECT_EQ(portApi->getAttribute(portIds[2], speed_attr), 50000);
  EXPECT_EQ(portApi->getAttribute(portIds[3], speed_attr), 25000);
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
    portApi->remove((portId));
  }
  {
    // remove rvalue id
    portApi->remove(createPort(25000, {42}, true));
  }
  {
    // remove in "canonical" for-loop
    auto portIds = createFivePorts();
    for (const auto& portId : portIds) {
      portApi->remove(portId);
    }
  }
}

TEST_F(PortApiTest, getLaneListPresized) {
  std::vector<uint32_t> inLanes{0, 1, 2, 3};
  auto portId = createPort(100000, inLanes, true);
  std::vector<uint32_t> tempLanes;
  tempLanes.resize(4);
  SaiPortTraits::Attributes::HwLaneList hwLaneListAttr{tempLanes};
  auto gotLanes = portApi->getAttribute(portId, hwLaneListAttr);
  EXPECT_EQ(gotLanes, inLanes);
}

TEST_F(PortApiTest, getLaneListUnsized) {
  std::vector<uint32_t> inLanes{0, 1, 2, 3};
  auto portId = createPort(100000, inLanes, true);
  SaiPortTraits::Attributes::HwLaneList hwLaneListAttr;
  auto gotLanes = portApi->getAttribute(portId, hwLaneListAttr);
  EXPECT_EQ(gotLanes, inLanes);
}

TEST_F(PortApiTest, setGetOptionalAttributes) {
  auto portId = createPort(25000, {42}, true);

  // Fec Mode get/set
  int32_t saiFecMode = SAI_PORT_FEC_MODE_RS;
  SaiPortTraits::Attributes::FecMode fecMode{saiFecMode};
  portApi->setAttribute(portId, fecMode);
  auto gotFecMode = portApi->getAttribute(portId, fecMode);
  EXPECT_EQ(gotFecMode, saiFecMode);

  // Internal Loopback Mode get/set
  int32_t saiInternalLoopbackMode = SAI_PORT_INTERNAL_LOOPBACK_MODE_MAC;
  SaiPortTraits::Attributes::InternalLoopbackMode internalLoopbackMode{
      saiInternalLoopbackMode};
  portApi->setAttribute(portId, internalLoopbackMode);
  auto gotInternalLoopbackMode =
      portApi->getAttribute(portId, internalLoopbackMode);
  EXPECT_EQ(gotInternalLoopbackMode, saiInternalLoopbackMode);

  // Media type get/set
  int32_t saiMediaType = SAI_PORT_MEDIA_TYPE_COPPER;
  SaiPortTraits::Attributes::MediaType mediaType{saiMediaType};
  portApi->setAttribute(portId, mediaType);
  auto gotMediaType = portApi->getAttribute(portId, mediaType);
  EXPECT_EQ(gotMediaType, saiMediaType);

  // Global Flow Control get/set
  int32_t saiFlowControl = SAI_PORT_FLOW_CONTROL_MODE_RX_ONLY;
  SaiPortTraits::Attributes::GlobalFlowControlMode flowControl{saiFlowControl};
  portApi->setAttribute(portId, flowControl);
  auto gotFlowControl = portApi->getAttribute(portId, flowControl);
  EXPECT_EQ(gotFlowControl, saiFlowControl);

  // Ingress port vlan get/set
  sai_vlan_id_t saiPortVlanId = 2000;
  SaiPortTraits::Attributes::PortVlanId portVlanId{saiPortVlanId};
  portApi->setAttribute(portId, portVlanId);
  auto gotPortVlanId = portApi->getAttribute(portId, portVlanId);
  EXPECT_EQ(gotPortVlanId, saiPortVlanId);
}

// ObjectApi tests
TEST_F(PortApiTest, portCount) {
  uint32_t count = getObjectCount<SaiPortTraits>(0);
  // cpu port
  EXPECT_EQ(count, 1);
  createFivePorts();
  count = getObjectCount<SaiPortTraits>(0);
  EXPECT_EQ(count, 6);
}

TEST_F(PortApiTest, portKeys) {
  auto portIds = createFivePorts();
  auto keys = getObjectKeys<SaiPortTraits>(0);
  EXPECT_EQ(keys.size(), 6);
  // cpu port
  portIds.push_back(PortSaiId(0));
  std::sort(portIds.begin(), portIds.end());
  std::sort(keys.begin(), keys.end());
  EXPECT_EQ(keys, portIds);
}
