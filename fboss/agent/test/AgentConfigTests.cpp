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

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/gen-cpp2/agent_config_types.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>

using namespace facebook::fboss;

namespace {

cfg::SwitchConfig createSwitchConfig() {
  cfg::SwitchConfig config;
  config.vlans_ref()->resize(2);
  *config.vlans_ref()[0].name_ref() = "PrimaryVlan";
  *config.vlans_ref()[0].id_ref() = 5;
  *config.vlans_ref()[0].routable_ref() = true;
  config.vlans_ref()[0].intfID_ref() = 1234;
  *config.vlans_ref()[1].name_ref() = "DefaultHWVlan";
  *config.vlans_ref()[1].id_ref() = 1;
  *config.vlans_ref()[1].routable_ref() = true;
  config.vlans_ref()[1].intfID_ref() = 4321;

  config.vlanPorts_ref()->resize(10);
  config.ports_ref()->resize(10);
  for (int n = 0; n < 10; ++n) {
    *config.ports_ref()[n].logicalID_ref() = n + 1;
    *config.ports_ref()[n].state_ref() = cfg::PortState::ENABLED;
    *config.ports_ref()[n].minFrameSize_ref() = 64;
    *config.ports_ref()[n].maxFrameSize_ref() = 9000;
    *config.ports_ref()[n].routable_ref() = true;
    *config.ports_ref()[n].ingressVlan_ref() = 5;

    *config.vlanPorts_ref()[n].vlanID_ref() = 5;
    *config.vlanPorts_ref()[n].logicalPort_ref() = n + 1;
    *config.vlanPorts_ref()[n].spanningTreeState_ref() =
        cfg::SpanningTreeState::FORWARDING;
    *config.vlanPorts_ref()[n].emitTags_ref() = 0;
  }

  config.interfaces_ref()->resize(2);
  *config.interfaces_ref()[0].intfID_ref() = 1234;
  *config.interfaces_ref()[0].vlanID_ref() = 5;
  config.interfaces_ref()[0].name_ref() = "PrimaryInterface";
  config.interfaces_ref()[0].mtu_ref() = 9000;
  config.interfaces_ref()[0].ipAddresses_ref()->resize(5);
  config.interfaces_ref()[0].ipAddresses_ref()[0] = "10.164.4.10/24";
  config.interfaces_ref()[0].ipAddresses_ref()[1] = "10.164.4.1/24";
  config.interfaces_ref()[0].ipAddresses_ref()[2] = "10.164.4.2/24";
  config.interfaces_ref()[0].ipAddresses_ref()[3] = "2401:db00:2110:3004::/64";
  config.interfaces_ref()[0].ipAddresses_ref()[4] =
      "2401:db00:2110:3004::000a/64";
  config.interfaces_ref()[0].ndp_ref() = cfg::NdpConfig();
  *config.interfaces_ref()[0].ndp_ref()->routerAdvertisementSeconds_ref() = 1;
  *config.interfaces_ref()[1].intfID_ref() = 4321;
  *config.interfaces_ref()[1].vlanID_ref() = 1;
  config.interfaces_ref()[1].name_ref() = "DefaultHWInterface";
  config.interfaces_ref()[1].mtu_ref() = 9000;
  config.interfaces_ref()[1].ipAddresses_ref()->resize(0);
  config.interfaces_ref()[1].ndp_ref() = cfg::NdpConfig();
  *config.interfaces_ref()[1].ndp_ref()->routerAdvertisementSeconds_ref() = 1;
  *config.arpTimeoutSeconds_ref() = 1;
  return config;
}

cfg::AgentConfig createAgentConfig() {
  cfg::AgentConfig config;
  *config.sw_ref() = createSwitchConfig();
  return config;
}

} // namespace

TEST(AgentConfigTest, AcceptsNewAndOldStyleConfigs) {
  auto oldStyle = createSwitchConfig();
  auto newStyle = createAgentConfig();

  auto oldStyleRaw =
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(oldStyle);
  auto newStyleRaw =
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(newStyle);

  auto fromOldStyle = AgentConfig::fromRawConfig(oldStyleRaw);
  auto fromNewStyle = AgentConfig::fromRawConfig(oldStyleRaw);

  EXPECT_EQ(fromOldStyle->swConfigRaw(), fromNewStyle->swConfigRaw());
}
