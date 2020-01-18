/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmPortUtils.h"

#include "fboss/agent/FbossError.h"

#include <thrift/lib/cpp/util/EnumUtils.h>

namespace facebook::fboss {

const PortSpeed2TransmitterTechAndMode& getSpeedToTransmitterTechAndMode() {
  // This allows mapping from a speed and port transmission technology
  // to a broadcom supported interface
  static const PortSpeed2TransmitterTechAndMode kPortTypeMapping = {
      {cfg::PortSpeed::HUNDREDG,
       {{TransmitterTechnology::COPPER, BCM_PORT_IF_CR4},
        {TransmitterTechnology::OPTICAL, BCM_PORT_IF_CAUI},
        // What to default to
        {TransmitterTechnology::UNKNOWN, BCM_PORT_IF_CAUI}}},
      {cfg::PortSpeed::FIFTYG,
       {{TransmitterTechnology::COPPER, BCM_PORT_IF_CR2},
        {TransmitterTechnology::OPTICAL, BCM_PORT_IF_CAUI},
        // What to default to
        {TransmitterTechnology::UNKNOWN, BCM_PORT_IF_CR2}}},
      {cfg::PortSpeed::FORTYG,
       {{TransmitterTechnology::COPPER, BCM_PORT_IF_CR4},
        {TransmitterTechnology::OPTICAL, BCM_PORT_IF_XLAUI},
        // What to default to
        {TransmitterTechnology::UNKNOWN, BCM_PORT_IF_XLAUI}}},
      {cfg::PortSpeed::TWENTYFIVEG,
       {{TransmitterTechnology::COPPER, BCM_PORT_IF_CR},
        {TransmitterTechnology::OPTICAL, BCM_PORT_IF_CAUI},
        // What to default to
        {TransmitterTechnology::UNKNOWN, BCM_PORT_IF_CR}}},
      {cfg::PortSpeed::TWENTYG,
       {{TransmitterTechnology::COPPER, BCM_PORT_IF_CR},
        // We don't expect 20G optics
        // What to default to
        {TransmitterTechnology::UNKNOWN, BCM_PORT_IF_CR}}},
      {cfg::PortSpeed::XG,
       {{TransmitterTechnology::COPPER, BCM_PORT_IF_CR},
        {TransmitterTechnology::OPTICAL, BCM_PORT_IF_SFI},
        // What to default to
        {TransmitterTechnology::UNKNOWN, BCM_PORT_IF_CR}}},
      {cfg::PortSpeed::GIGE,
       {{TransmitterTechnology::COPPER, BCM_PORT_IF_GMII},
        // We don't expect 1G optics
        // What to default to
        {TransmitterTechnology::UNKNOWN, BCM_PORT_IF_GMII}}}};
  return kPortTypeMapping;
}

uint32_t getDesiredPhyLaneConfig(
    TransmitterTechnology tech,
    cfg::PortSpeed speed) {
  // see shared/port.h for what these various shifts mean. I elide
  // them here simply because they are very verbose.
  if (tech == TransmitterTechnology::BACKPLANE) {
    if (speed == cfg::PortSpeed::FORTYG) {
      // DFE + BACKPLANE + NRZ
      return 0x8004;
    } else if (speed == cfg::PortSpeed::HUNDREDG) {
      // DFE + BACKPLANE + PAM4 + NS
      return 0x5004;
    }
  } else if (tech == TransmitterTechnology::COPPER) {
    if (speed == cfg::PortSpeed::FORTYG) {
      // DFE + COPPER + NRZ
      return 0x8024;
    } else if (speed == cfg::PortSpeed::HUNDREDG) {
      // DFE + COPPER + PAM4 + NS
      return 0x5024;
    }
  }
  // TODO: add more mappings + better error message
  throw std::runtime_error("Unsupported tech+speed in port_resource");
}

} // namespace facebook::fboss

namespace facebook::fboss::utility {

bcm_port_phy_fec_t phyFecModeToBcmPortPhyFec(phy::FecMode fec) {
  switch (fec) {
    case phy::FecMode::NONE:
      return bcmPortPhyFecNone;
    case phy::FecMode::CL74:
      return bcmPortPhyFecBaseR;
    case phy::FecMode::CL91:
      return bcmPortPhyFecRsFec;
    case phy::FecMode::RS528:
      return bcmPortPhyFecRsFec;
    case phy::FecMode::RS544:
      return bcmPortPhyFecRs544;
    case phy::FecMode::RS544_2N:
      return bcmPortPhyFecRs544_2xN;
  };
  throw facebook::fboss::FbossError(
      "Unsupported fec type: ", apache::thrift::util::enumNameSafe(fec));
}

} // namespace facebook::fboss::utility
