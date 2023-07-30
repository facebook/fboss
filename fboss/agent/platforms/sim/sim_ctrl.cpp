/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <folly/Memory.h>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Main.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/sim/SimPlatform.h"

#include <gflags/gflags.h>

using namespace facebook::fboss;
using folly::MacAddress;
using std::make_unique;
using std::unique_ptr;

DEFINE_int32(num_ports, 64, "The number of ports in the simulated switch");
DEFINE_string(
    local_mac,
    "02:00:00:00:00:01",
    "The local MAC address to use for the switch");

namespace facebook::fboss {

unique_ptr<Platform> initSimPlatform(
    std::unique_ptr<AgentConfig> config,
    uint32_t hwFeaturesDesired,
    int16_t switchIndex) {
  // Disable the tun interface code by default.
  // We normally don't want the sim switch to create real interfaces
  // on the host.
  gflags::SetCommandLineOptionWithMode(
      "tun_intf", "no", gflags::SET_FLAGS_DEFAULT);

  MacAddress localMac(FLAGS_local_mac);
  auto platform = make_unique<SimPlatform>(localMac, FLAGS_num_ports);
  platform->init(std::move(config), hwFeaturesDesired, switchIndex);
  return std::move(platform);
}

} // namespace facebook::fboss

int main(int argc, char* argv[]) {
  setVersionInfo();
  auto config = fbossCommonInit(argc, argv);
  auto fbossInitializer = std::make_unique<MonolithicAgentInitializer>(
      std::move(config),
      (HwSwitch::FeaturesDesired::PACKET_RX_DESIRED |
       HwSwitch::FeaturesDesired::LINKSCAN_DESIRED),
      initSimPlatform);
  return facebook::fboss::fbossMain(argc, argv, std::move(fbossInitializer));
}
