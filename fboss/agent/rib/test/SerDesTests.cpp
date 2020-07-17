/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddress.h>
#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/json.h>
#include <gtest/gtest.h>

using namespace facebook::fboss;

cfg::SwitchConfig interfaceAndStaticRoutesWithNextHopsConfig() {
  cfg::SwitchConfig config;
  config.vlans.resize(2);
  config.vlans[0].id = 1;
  config.vlans[1].id = 2;

  config.interfaces.resize(2);
  config.interfaces[0].intfID = 1;
  config.interfaces[0].vlanID = 1;
  config.interfaces[0].routerID = 0;
  config.interfaces[0].mac_ref() = "00:00:00:00:00:11";
  config.interfaces[0].ipAddresses.resize(2);
  config.interfaces[0].ipAddresses[0] = "1.1.1.1/24";
  config.interfaces[0].ipAddresses[1] = "1::1/48";
  config.interfaces[1].intfID = 2;
  config.interfaces[1].vlanID = 2;
  config.interfaces[1].routerID = 0;
  config.interfaces[1].mac_ref() = "00:00:00:00:00:22";
  config.interfaces[1].ipAddresses.resize(2);
  config.interfaces[1].ipAddresses[0] = "2.2.2.2/24";
  config.interfaces[1].ipAddresses[1] = "2::1/48";

  config.staticRoutesWithNhops.resize(2);
  config.staticRoutesWithNhops[0].nexthops.resize(1);
  config.staticRoutesWithNhops[0].prefix = "2001::/64";
  config.staticRoutesWithNhops[0].nexthops[0] = "2::2";
  config.staticRoutesWithNhops[1].nexthops.resize(1);
  config.staticRoutesWithNhops[1].prefix = "20.20.20.0/24";
  config.staticRoutesWithNhops[1].nexthops[0] = "2.2.2.3";

  return config;
}

TEST(WarmBoot, Serialization) {
  rib::RoutingInformationBase rib;

  auto emptyState = std::make_shared<SwitchState>();
  auto platform = createMockPlatform();
  auto config = interfaceAndStaticRoutesWithNextHopsConfig();

  auto state = publishAndApplyConfig(emptyState, &config, platform.get(), &rib);
  ASSERT_NE(nullptr, state);

  auto serializedRib = rib.toFollyDynamic();
  auto deserializedRib =
      rib::RoutingInformationBase::fromFollyDynamic(serializedRib);

  ASSERT_EQ(rib, deserializedRib);
}
