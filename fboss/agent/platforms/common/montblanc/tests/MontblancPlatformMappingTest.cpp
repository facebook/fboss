/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/montblanc/MontblancPlatformMapping.h"
#include "fboss/agent/platforms/common/montblanc/MontblancPrecodingPlatformMapping.h"

#include <set>

#include <folly/String.h>
#include <gtest/gtest.h>

namespace facebook::fboss {
namespace {

// Downlink (host/NIC-facing) transceivers for a San Miguel/VR200 montblanc RSW:
// eth1/33-63 excluding every 4th port. These are the only ports that should
// carry precoding in the precoding variant.
std::set<int> downlinkTransceivers() {
  std::set<int> tcvrs;
  for (int t = 33; t <= 63; ++t) {
    if (t % 4 != 0) {
      tcvrs.insert(t);
    }
  }
  return tcvrs;
}

int transceiverOf(const std::string& portName) {
  // Port names look like "eth1/<tcvr>/<subport>"; the transceiver is the
  // second '/'-separated field.
  std::vector<std::string_view> parts;
  folly::split('/', portName, parts);
  return folly::to<int>(parts.at(1));
}

// Transceivers that have TX or RX precoding set on any supported profile pin.
std::set<int> precodedTransceivers(const PlatformMapping& mapping) {
  std::set<int> tcvrs;
  for (const auto& [portId, portEntry] : mapping.getPlatformPorts()) {
    for (const auto& [profile, portConfig] : *portEntry.supportedProfiles()) {
      for (const auto& pin : *portConfig.pins()->iphy()) {
        bool precoded =
            (pin.tx().has_value() && pin.tx()->precoding().has_value()) ||
            (pin.rx().has_value() && pin.rx()->precoding().has_value());
        if (precoded) {
          tcvrs.insert(transceiverOf(*portEntry.mapping()->name()));
        }
      }
    }
  }
  return tcvrs;
}

// Transceivers that have RX_REACH (rxReach) set on any supported profile pin.
std::set<int> extendedReachTransceivers(const PlatformMapping& mapping) {
  std::set<int> tcvrs;
  for (const auto& [portId, portEntry] : mapping.getPlatformPorts()) {
    for (const auto& [profile, portConfig] : *portEntry.supportedProfiles()) {
      for (const auto& pin : *portConfig.pins()->iphy()) {
        if (pin.rx().has_value() && pin.rx()->rxReach().has_value()) {
          tcvrs.insert(transceiverOf(*portEntry.mapping()->name()));
        }
      }
    }
  }
  return tcvrs;
}

} // namespace

// The base montblanc mapping must construct with the default flags
// (FLAGS_montblanc_precoding defaults to false) and must not enable precoding
// anywhere.
TEST(MontblancPlatformMappingTest, defaultConstructsWithoutPrecoding) {
  MontblancPlatformMapping mapping;
  EXPECT_FALSE(mapping.getPlatformPorts().empty());
  EXPECT_TRUE(precodedTransceivers(mapping).empty());
  EXPECT_TRUE(extendedReachTransceivers(mapping).empty());
}

// The precoding variant blob must parse and enable precoding and RX_REACH on
// exactly the downlink transceivers, never on uplink ports.
TEST(MontblancPlatformMappingTest, precodingVariantPrecodesDownlinksOnly) {
  MontblancPlatformMapping mapping(kJsonPrecodingPlatformMappingStr);
  EXPECT_FALSE(mapping.getPlatformPorts().empty());
  EXPECT_EQ(precodedTransceivers(mapping), downlinkTransceivers());
  EXPECT_EQ(extendedReachTransceivers(mapping), downlinkTransceivers());
}

} // namespace facebook::fboss
