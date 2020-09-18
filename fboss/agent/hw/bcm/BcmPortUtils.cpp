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
#include "fboss/agent/hw/bcm/BcmError.h"

#include <folly/logging/xlog.h>
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

phy::FecMode bcmPortPhyFecToPhyFecMode(bcm_port_phy_fec_t fec) {
  switch (fec) {
    case bcmPortPhyFecNone:
      return phy::FecMode::NONE;
    case bcmPortPhyFecBaseR:
      return phy::FecMode::CL74;
    case bcmPortPhyFecRsFec:
      return phy::FecMode::RS528;
    case bcmPortPhyFecRs544:
      return phy::FecMode::RS544;
    case bcmPortPhyFecRs544_2xN:
      return phy::FecMode::RS544_2N;
    default:
      throw facebook::fboss::FbossError("Unsupported fec type: ", fec);
  };
}

uint32_t getDesiredPhyLaneConfig(const phy::PortProfileConfig& profileCfg) {
  uint32_t laneConfig = 0;

  // PortResource setting needs interface mode from profile config
  uint32_t medium = 0;
  if (auto interfaceMode = profileCfg.get_iphy().interfaceMode_ref()) {
    switch (*interfaceMode) {
      case phy::InterfaceMode::KR:
      case phy::InterfaceMode::KR2:
      case phy::InterfaceMode::KR4:
      case phy::InterfaceMode::KR8:
      case phy::InterfaceMode::CAUI4_C2C:
      case phy::InterfaceMode::CAUI4_C2M:
        medium = BCM_PORT_RESOURCE_PHY_LANE_CONFIG_MEDIUM_BACKPLANE;
        break;
      case phy::InterfaceMode::CR:
      case phy::InterfaceMode::CR2:
      case phy::InterfaceMode::CR4:
        medium = BCM_PORT_RESOURCE_PHY_LANE_CONFIG_MEDIUM_COPPER_CABLE;
        break;
      case phy::InterfaceMode::SR:
      case phy::InterfaceMode::SR4:
        medium = BCM_PORT_RESOURCE_PHY_LANE_CONFIG_MEDIUM_OPTICS;
        break;
      default:
        throw facebook::fboss::FbossError(
            "Unsupported interface mode: ",
            apache::thrift::util::enumNameSafe(*interfaceMode));
    }
  } else {
    throw facebook::fboss::FbossError(
        "No iphy interface mode in profileCfg for speed: ",
        apache::thrift::util::enumNameSafe(profileCfg.get_speed()));
  }
  BCM_PORT_RESOURCE_PHY_LANE_CONFIG_MEDIUM_SET(laneConfig, medium);

  switch (profileCfg.get_iphy().get_modulation()) {
    case phy::IpModulation::PAM4:
      // PAM4 + NS
      BCM_PORT_RESOURCE_PHY_LANE_CONFIG_FORCE_PAM4_SET(laneConfig);
      BCM_PORT_RESOURCE_PHY_LANE_CONFIG_FORCE_NS_SET(laneConfig);
      break;
    case phy::IpModulation::NRZ:
      // NRZ
      BCM_PORT_RESOURCE_PHY_LANE_CONFIG_FORCE_NRZ_SET(laneConfig);
      break;
  };

  // always enable DFE
  BCM_PORT_RESOURCE_PHY_LANE_CONFIG_DFE_SET(laneConfig);

  return laneConfig;
}

bcm_gport_t getPortGport(int unit, int port) {
  bcm_gport_t portGport;
  auto rv = bcm_port_gport_get(unit, port, &portGport);
  facebook::fboss::bcmCheckError(rv, "failed to get gport for port");
  return portGport;
}

bcm_port_loopback_t fbToBcmLoopbackMode(cfg::PortLoopbackMode inMode) {
  switch (inMode) {
    case cfg::PortLoopbackMode::NONE:
      return BCM_PORT_LOOPBACK_NONE;
    case cfg::PortLoopbackMode::PHY:
      return BCM_PORT_LOOPBACK_PHY;
    case cfg::PortLoopbackMode::MAC:
      return BCM_PORT_LOOPBACK_MAC;
  }
  CHECK(0) << "Should never reach here";
  return BCM_PORT_LOOPBACK_NONE;
}

} // namespace facebook::fboss::utility
