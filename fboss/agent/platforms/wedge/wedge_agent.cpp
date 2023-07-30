/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Main.h"
#include "fboss/agent/platforms/wedge/WedgePlatformInit.h"

using namespace facebook::fboss;

int main(int argc, char* argv[]) {
  setVersionInfo();
  auto config = fbossCommonInit(argc, argv);
  auto fbossInitializer = std::make_unique<MonolithicAgentInitializer>(
      std::move(config),
      (HwSwitch::FeaturesDesired::PACKET_RX_DESIRED |
       HwSwitch::FeaturesDesired::LINKSCAN_DESIRED),
      initWedgePlatform);
  return facebook::fboss::fbossMain(argc, argv, std::move(fbossInitializer));
}
