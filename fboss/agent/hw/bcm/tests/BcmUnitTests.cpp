/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/BcmPortGroup.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/types.h"

using facebook::fboss::BcmPortGroup;
using facebook::fboss::Port;
using facebook::fboss::PortFields;
using facebook::fboss::PortID;
using facebook::fboss::cfg::PortSpeed;
using facebook::fboss::cfg::PortState;

namespace {

struct PortInfo {
  PortID id;
  PortSpeed speed;
  PortState state;
};

struct PortData {
  std::vector<std::shared_ptr<Port>> ports;

  void addPort(const PortInfo& info) {
    auto port =
        std::make_shared<Port>(info.id, "test" + std::to_string(info.id));
    port->setSpeed(info.speed);
    port->setAdminState(info.state);
    ports.push_back(port);
  }

  PortData(const std::initializer_list<PortInfo>& ports) {
    for (auto port : ports) {
      addPort(port);
    }
  }
};

PortData get4x10gPorts() {
  return {{PortID(0), PortSpeed::XG, PortState::ENABLED},
          {PortID(1), PortSpeed::XG, PortState::ENABLED},
          {PortID(2), PortSpeed::XG, PortState::ENABLED},
          {PortID(3), PortSpeed::XG, PortState::ENABLED}};
}

PortData get2x20gPorts() {
  return {{PortID(0), PortSpeed::TWENTYG, PortState::ENABLED},
          {PortID(1), PortSpeed::XG, PortState::DISABLED},
          {PortID(2), PortSpeed::TWENTYG, PortState::ENABLED},
          {PortID(3), PortSpeed::XG, PortState::DISABLED}};
}

PortData get1x40gPorts() {
  return {{PortID(0), PortSpeed::FORTYG, PortState::ENABLED},
          {PortID(1), PortSpeed::XG, PortState::DISABLED},
          {PortID(2), PortSpeed::XG, PortState::DISABLED},
          {PortID(3), PortSpeed::XG, PortState::DISABLED}};
}

PortData get1x10gPorts() {
  return {{PortID(0), PortSpeed::XG, PortState::ENABLED},
          {PortID(1), PortSpeed::XG, PortState::DISABLED},
          {PortID(2), PortSpeed::XG, PortState::DISABLED},
          {PortID(3), PortSpeed::XG, PortState::DISABLED}};
}

PortData getInvalid1x40gPorts() {
  return {{PortID(0), PortSpeed::XG, PortState::DISABLED},
          {PortID(1), PortSpeed::FORTYG, PortState::ENABLED},
          {PortID(2), PortSpeed::XG, PortState::DISABLED},
          {PortID(3), PortSpeed::XG, PortState::DISABLED}};
}

PortData getInvalid20gPorts() {
  return {{PortID(0), PortSpeed::TWENTYG, PortState::ENABLED},
          {PortID(1), PortSpeed::XG, PortState::ENABLED},
          {PortID(2), PortSpeed::XG, PortState::ENABLED},
          {PortID(3), PortSpeed::XG, PortState::DISABLED}};
}

PortData getInvalidDefaultPorts() {
  return {{PortID(0), PortSpeed::DEFAULT, PortState::ENABLED},
          {PortID(1), PortSpeed::DEFAULT, PortState::ENABLED},
          {PortID(2), PortSpeed::DEFAULT, PortState::ENABLED},
          {PortID(3), PortSpeed::DEFAULT, PortState::ENABLED}};
}

} // namespace

TEST(BcmUnitTests, FlexportCalculate) {
  // 4x10
  auto data = get4x10gPorts();
  auto desired =
      BcmPortGroup::calculateDesiredLaneMode(data.ports, {PortSpeed::XG});
  EXPECT_EQ(desired, BcmPortGroup::LaneMode::SINGLE);

  data = get2x20gPorts();
  desired = BcmPortGroup::calculateDesiredLaneMode(data.ports, {PortSpeed::XG});
  EXPECT_EQ(desired, BcmPortGroup::LaneMode::DUAL);

  data = get1x40gPorts();
  desired = BcmPortGroup::calculateDesiredLaneMode(data.ports, {PortSpeed::XG});
  EXPECT_EQ(desired, BcmPortGroup::LaneMode::QUAD);

  data = get1x10gPorts();
  desired = BcmPortGroup::calculateDesiredLaneMode(data.ports, {PortSpeed::XG});
  EXPECT_EQ(desired, BcmPortGroup::LaneMode::SINGLE);

  data = getInvalid1x40gPorts();
  EXPECT_ANY_THROW(
      BcmPortGroup::calculateDesiredLaneMode(data.ports, {PortSpeed::XG}));

  data = getInvalid20gPorts();
  EXPECT_ANY_THROW(
      BcmPortGroup::calculateDesiredLaneMode(data.ports, {PortSpeed::XG}));

  data = getInvalidDefaultPorts();
  EXPECT_ANY_THROW(
      BcmPortGroup::calculateDesiredLaneMode(data.ports, {PortSpeed::XG}));
}
