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
#include <thrift/lib/cpp/util/EnumUtils.h>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/BcmPortGroup.h"
#include "fboss/agent/platforms/common/wedge100/Wedge100PlatformMapping.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/types.h"

using facebook::fboss::BcmPortGroup;
using facebook::fboss::PlatformMapping;
using facebook::fboss::PlatformPortProfileConfigMatcher;
using facebook::fboss::Port;
using facebook::fboss::PortID;
using facebook::fboss::Wedge100PlatformMapping;
using facebook::fboss::cfg::PortProfileID;
using facebook::fboss::cfg::PortSpeed;
using facebook::fboss::cfg::PortState;
using facebook::fboss::state::PortFields;

namespace {
const auto kPlatformMapping = std::make_unique<Wedge100PlatformMapping>();

struct PortInfo {
  PortID id;
  PortProfileID profileID;
  PortState state;
};

struct PortData {
  std::vector<std::shared_ptr<Port>> ports;

  void addPort(const PortInfo& info) {
    if (auto itProfile = kPlatformMapping->getPortProfileConfig(
            PlatformPortProfileConfigMatcher(info.profileID, info.id));
        itProfile.has_value() ||
        info.profileID == PortProfileID::PROFILE_DEFAULT) {
      PortFields portFields;
      portFields.portId() = info.id;
      portFields.portName() = folly::to<std::string>("test", info.id);
      auto port = std::make_shared<Port>(std::move(portFields));
      port->setAdminState(info.state);
      port->setProfileId(info.profileID);
      if (info.profileID == PortProfileID::PROFILE_DEFAULT) {
        port->setSpeed(PortSpeed::DEFAULT);
      } else {
        port->setSpeed(*itProfile->speed());
      }
      ports.push_back(port);
    } else {
      throw facebook::fboss::FbossError(
          "Unsupported profile: ",
          apache::thrift::util::enumNameSafe(info.profileID));
    }
  }

  PortData(const std::initializer_list<PortInfo>& ports) {
    for (auto port : ports) {
      addPort(port);
    }
  }
};

PortData get4x10gPorts() {
  return {
      {PortID(1),
       PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
       PortState::ENABLED},
      {PortID(2),
       PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
       PortState::ENABLED},
      {PortID(3),
       PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
       PortState::ENABLED},
      {PortID(4),
       PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
       PortState::ENABLED}};
}

PortData get2x20gPorts() {
  return {
      {PortID(1),
       PortProfileID::PROFILE_20G_2_NRZ_NOFEC_COPPER,
       PortState::ENABLED},
      {PortID(2),
       PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
       PortState::DISABLED},
      {PortID(3),
       PortProfileID::PROFILE_20G_2_NRZ_NOFEC_COPPER,
       PortState::ENABLED},
      {PortID(4),
       PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
       PortState::DISABLED}};
}

PortData get1x40gPorts() {
  return {
      {PortID(1),
       PortProfileID::PROFILE_40G_4_NRZ_NOFEC_COPPER,
       PortState::ENABLED},
      {PortID(2),
       PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
       PortState::DISABLED},
      {PortID(3),
       PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
       PortState::DISABLED},
      {PortID(4),
       PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
       PortState::DISABLED}};
}

PortData get1x10gPorts() {
  return {
      {PortID(1),
       PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
       PortState::ENABLED},
      {PortID(2),
       PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
       PortState::DISABLED},
      {PortID(3),
       PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
       PortState::DISABLED},
      {PortID(4),
       PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
       PortState::DISABLED}};
}

PortData getInvalid1x40gPorts() {
  return {
      {PortID(1),
       PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
       PortState::DISABLED},
      {PortID(2),
       PortProfileID::PROFILE_40G_4_NRZ_NOFEC_COPPER,
       PortState::ENABLED},
      {PortID(3),
       PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
       PortState::DISABLED},
      {PortID(4),
       PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
       PortState::DISABLED}};
}

PortData getInvalid20gPorts() {
  return {
      {PortID(1),
       PortProfileID::PROFILE_20G_2_NRZ_NOFEC_COPPER,
       PortState::ENABLED},
      {PortID(2),
       PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
       PortState::ENABLED},
      {PortID(3),
       PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
       PortState::ENABLED},
      {PortID(4),
       PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
       PortState::DISABLED}};
}

PortData getInvalidDefaultPorts() {
  return {
      {PortID(1), PortProfileID::PROFILE_DEFAULT, PortState::ENABLED},
      {PortID(2), PortProfileID::PROFILE_DEFAULT, PortState::ENABLED},
      {PortID(3), PortProfileID::PROFILE_DEFAULT, PortState::ENABLED},
      {PortID(4), PortProfileID::PROFILE_DEFAULT, PortState::ENABLED}};
}

} // namespace

TEST(BcmUnitTests, FlexportCalculate) {
  // 4x10
  auto data = get4x10gPorts();
  auto desired = BcmPortGroup::calculateDesiredLaneMode(
      data.ports, kPlatformMapping.get());
  EXPECT_EQ(desired, BcmPortGroup::LaneMode::SINGLE);

  data = get2x20gPorts();
  desired = BcmPortGroup::calculateDesiredLaneMode(
      data.ports, kPlatformMapping.get());
  EXPECT_EQ(desired, BcmPortGroup::LaneMode::DUAL);

  data = get1x40gPorts();
  desired = BcmPortGroup::calculateDesiredLaneMode(
      data.ports, kPlatformMapping.get());
  EXPECT_EQ(desired, BcmPortGroup::LaneMode::QUAD);

  data = get1x10gPorts();
  desired = BcmPortGroup::calculateDesiredLaneMode(
      data.ports, kPlatformMapping.get());
  EXPECT_EQ(desired, BcmPortGroup::LaneMode::SINGLE);

  data = getInvalid1x40gPorts();
  EXPECT_ANY_THROW(BcmPortGroup::calculateDesiredLaneMode(
      data.ports, kPlatformMapping.get()));

  data = getInvalid20gPorts();
  EXPECT_ANY_THROW(BcmPortGroup::calculateDesiredLaneMode(
      data.ports, kPlatformMapping.get()));

  data = getInvalidDefaultPorts();
  EXPECT_ANY_THROW(BcmPortGroup::calculateDesiredLaneMode(
      data.ports, kPlatformMapping.get()));
}
