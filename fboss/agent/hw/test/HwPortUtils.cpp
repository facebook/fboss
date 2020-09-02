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

std::vector<cfg::PortProfileID> allPortProfiles() {
  return {
      cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_25G_1_NRZ_CL74_COPPER,
      cfg::PortProfileID::PROFILE_25G_1_NRZ_RS528_COPPER,
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_50G_2_NRZ_CL74_COPPER,
      cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_OPTICAL,
      cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_COPPER,
      cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_OPTICAL,
      cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_COPPER,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL,
      cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC_COPPER,
  };
}

cfg::PortSpeed getSpeed(cfg::PortProfileID profile) {
  switch (profile) {
    case cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC:
    case cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER:
    case cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL:
      return cfg::PortSpeed::XG;

    case cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC:
    case cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC_OPTICAL:
    case cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC_COPPER:
      return cfg::PortSpeed::TWENTYG;

    case cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC:
    case cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_COPPER:
    case cfg::PortProfileID::PROFILE_25G_1_NRZ_CL74_COPPER:
    case cfg::PortProfileID::PROFILE_25G_1_NRZ_RS528_COPPER:
    case cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL:
      return cfg::PortSpeed::TWENTYFIVEG;

    case cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC:
    case cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_COPPER:
    case cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_OPTICAL:
      return cfg::PortSpeed::FORTYG;

    case cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC:
    case cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_COPPER:
    case cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_OPTICAL:
    case cfg::PortProfileID::PROFILE_50G_2_NRZ_CL74_COPPER:
    case cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER:
      return cfg::PortSpeed::FIFTYG;

    case cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_OPTICAL:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_COPPER:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC_COPPER:
      return cfg::PortSpeed::HUNDREDG;

    case cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N:
    case cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_COPPER:
    case cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_OPTICAL:
      return cfg::PortSpeed::TWOHUNDREDG;

    case cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N:
    case cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL:
      return cfg::PortSpeed::FOURHUNDREDG;

    case cfg::PortProfileID::PROFILE_DEFAULT:
      break;
  }
  return cfg::PortSpeed::DEFAULT;
}
TransmitterTechnology getMediaType(cfg::PortProfileID profile) {
  switch (profile) {
    case cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER:
    case cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC_COPPER:
    case cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_COPPER:
    case cfg::PortProfileID::PROFILE_25G_1_NRZ_CL74_COPPER:
    case cfg::PortProfileID::PROFILE_25G_1_NRZ_RS528_COPPER:
    case cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_COPPER:
    case cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_COPPER:
    case cfg::PortProfileID::PROFILE_50G_2_NRZ_CL74_COPPER:
    case cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_COPPER:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC_COPPER:
    case cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_COPPER:
      return TransmitterTechnology::COPPER;

    case cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL:
    case cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC_OPTICAL:
    case cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL:
    case cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_OPTICAL:
    case cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_OPTICAL:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_OPTICAL:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL:
    case cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_OPTICAL:
    case cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL:
      return TransmitterTechnology::OPTICAL;

    case cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC:
    case cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC:
    case cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC:
    case cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC:
    case cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528:
    case cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N:
    case cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N:
    case cfg::PortProfileID::PROFILE_DEFAULT:
      break;
  }
  return TransmitterTechnology::UNKNOWN;
}
} // namespace facebook::fboss::utility
