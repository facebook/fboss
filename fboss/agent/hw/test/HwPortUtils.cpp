/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwPortUtils.h"

#include "fboss/agent/FbossError.h"

namespace facebook::fboss::utility {

void updateFlexConfig(
    cfg::SwitchConfig* config,
    FlexPortMode flexMode,
    std::vector<PortID> allPortsinGroup,
    const Platform* platform) {
  if (flexMode == FlexPortMode::FOURX10G) {
    facebook::fboss::utility::enableAllLanes(
        config, cfg::PortSpeed::XG, allPortsinGroup, platform);
  } else if (flexMode == FlexPortMode::FOURX25G) {
    facebook::fboss::utility::enableAllLanes(
        config, cfg::PortSpeed::TWENTYFIVEG, allPortsinGroup, platform);
  } else if (flexMode == FlexPortMode::ONEX40G) {
    facebook::fboss::utility::enableOneLane(
        config,
        cfg::PortSpeed::FORTYG,
        cfg::PortSpeed::XG,
        allPortsinGroup,
        platform);
  } else if (flexMode == FlexPortMode::TWOX50G) {
    facebook::fboss::utility::enableTwoLanes(
        config,
        cfg::PortSpeed::FIFTYG,
        cfg::PortSpeed::TWENTYFIVEG,
        allPortsinGroup,
        platform);
  } else if (flexMode == FlexPortMode::ONEX100G) {
    facebook::fboss::utility::enableOneLane(
        config,
        cfg::PortSpeed::HUNDREDG,
        cfg::PortSpeed::TWENTYFIVEG,
        allPortsinGroup,
        platform);
  } else {
    throw FbossError("invalid FlexConfig Mode");
  }
}
} // namespace facebook::fboss::utility
