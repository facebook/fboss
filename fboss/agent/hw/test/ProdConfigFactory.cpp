/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/ProdConfigFactory.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/lib/platforms/PlatformMode.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestPfcUtils.h"
#include "fboss/agent/test/utils/DscpMarkingUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/QueuePerHostTestUtils.h"

namespace {
auto constexpr kTopLabel = 5000;
}

namespace facebook::fboss::utility {

/*
 * Setup and enable Olympic QoS on the given config. Common among all platforms;
 * most use only Olympic QoS, but some roles such as MHNIC RSWs use this
 * together with queue-per-host mapping.
 */
void addOlympicQosToConfig(
    cfg::SwitchConfig& config,
    const HwAsic* hwAsic,
    bool enableStrictPriority) {
  addOlympicQosMaps(config, {hwAsic});
  if (enableStrictPriority) {
    addFswRswAllSPOlympicQueueConfig(&config, {hwAsic});
  } else {
    addOlympicQueueConfig(&config, {hwAsic});
  }
}

void addOlympicQosToConfig(
    cfg::SwitchConfig& config,
    const HwSwitch* hwSwitch,
    bool enableStrictPriority) {
  addOlympicQosToConfig(
      config, hwSwitch->getPlatform()->getAsic(), enableStrictPriority);
}

void addNetworkAIQosToConfig(cfg::SwitchConfig& config, const HwAsic* hwAsic) {
  addNetworkAIQosMaps(config, {hwAsic});
  auto streamType =
      *hwAsic->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT).begin();
  // queue configuration is different
  addNetworkAIQueueConfig(&config, streamType);
}

void addNetworkAIQosToConfig(
    cfg::SwitchConfig& config,
    const HwSwitch* hwSwitch) {
  auto hwAsic = hwSwitch->getPlatform()->getAsic();
  addNetworkAIQosToConfig(config, hwAsic);
}

void addLoadBalancerToConfig(
    cfg::SwitchConfig& config,
    const HwSwitch* hwSwitch,
    LBHash hashType) {
  addLoadBalancerToConfig(config, hwSwitch->getPlatform()->getAsic(), hashType);
}

void addMplsConfig(cfg::SwitchConfig& config) {
  config.staticMplsRoutesWithNhops()->resize(1);
  config.staticMplsRoutesWithNhops()[0].ingressLabel() = kTopLabel;
  std::vector<NextHopThrift> nexthops;
  nexthops.resize(1);
  MplsAction swap;
  swap.action() = MplsActionCode::SWAP;
  swap.swapLabel() = 101;
  nexthops[0].mplsAction() = swap;
  nexthops[0].address() = network::toBinaryAddress(folly::IPAddress("1::2"));
  config.staticMplsRoutesWithNhops()[0].nexthops() = nexthops;
}

/*
 * Straightforward convenience function. Uses values defined in getUplinksCount,
 * from HwInitAndExitBenchmarkHelper.cpp, to return the number of uplinks for
 * each platform.
 */
uint16_t uplinksCountFromSwitch(PlatformType mode) {
  using PM = PlatformType;
  switch (mode) {
    case PM::PLATFORM_WEDGE:
    case PM::PLATFORM_WEDGE100:
    case PM::PLATFORM_WEDGE400C:
    case PM::PLATFORM_WEDGE400:
    case PM::PLATFORM_YAMP:
    case PM::PLATFORM_MORGAN800CC:
    case PM::PLATFORM_MINIPACK:
    case PM::PLATFORM_ELBERT:
    case PM::PLATFORM_FUJI:
    case PM::PLATFORM_CLOUDRIPPER:
    case PM::PLATFORM_GALAXY_LC:
    case PM::PLATFORM_GALAXY_FC:
    case PM::PLATFORM_DARWIN:
    case PM::PLATFORM_MONTBLANC:
      return 4;
    default:
      throw FbossError(
          "provided PlatformType: ",
          mode,
          " has not been defined for uplinksCountFromSwitch");
      break;
  }
}

uint16_t uplinksCountFromSwitch(const HwSwitch* hwSwitch) {
  return uplinksCountFromSwitch(hwSwitch->getPlatform()->getType());
}

cfg::PortSpeed getPortSpeed(
    cfg::AsicType hwAsicType,
    PlatformType platformType) {
  cfg::PortSpeed portSpeed = cfg::PortSpeed::DEFAULT;

  switch (hwAsicType) {
    case cfg::AsicType::ASIC_TYPE_TRIDENT2:
      portSpeed = cfg::PortSpeed::FORTYG;
      break;
    default:
      portSpeed = cfg::PortSpeed::HUNDREDG;
      break;
  }

  // override speed for certain platforms based on the
  // mode of the asic
  switch (platformType) {
    case PlatformType::PLATFORM_FUJI:
    case PlatformType::PLATFORM_ELBERT:
      portSpeed = cfg::PortSpeed::TWOHUNDREDG;
      break;
    case PlatformType::PLATFORM_MONTBLANC:
      portSpeed = cfg::PortSpeed::FOURHUNDREDG;
      break;
    default:
      /* do nothing */
      break;
  }
  if (portSpeed == cfg::PortSpeed::DEFAULT) {
    throw FbossError(
        "port speed not set for asic: ",
        hwAsicType,
        " platform mode: ",
        platformType);
  }
  return portSpeed;
}

cfg::PortSpeed getPortSpeed(const HwSwitch* hwSwitch) {
  return getPortSpeed(
      hwSwitch->getPlatform()->getAsic()->getAsicType(),
      hwSwitch->getPlatform()->getType());
}
/*
 * Adds queue-per-host mapping to config, based on HwQueuePerHostRouteTests.cpp.
 * Other helper functions (i.e. addRoutes, updateRoutesClassID) are called on
 * the test fixture's setup() phase.
 */
void addQueuePerHostToConfig(cfg::SwitchConfig& config, bool isSai) {
  utility::addQueuePerHostQueueConfig(&config);
  utility::addQueuePerHostAcls(&config, isSai);
}

cfg::SwitchConfig createProdRtswConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& masterLogicalPortIds,
    const HwSwitchEnsemble* ensemble) {
  auto platform = hwSwitch->getPlatform();
  auto hwAsic = platform->getAsic();

  // its the same speed used for the uplink and downlink for now
  auto portSpeed = getPortSpeed(hwSwitch);

  std::vector<PortID> uplinks, downlinks;
  // Create initial config to which we can add the rest of the features.
  auto config = createRtswUplinkDownlinkConfig(
      hwSwitch,
      const_cast<HwSwitchEnsemble*>(ensemble),
      masterLogicalPortIds,
      portSpeed,
      hwAsic->desiredLoopbackModes(),
      uplinks,
      downlinks);

  addCpuQueueConfig(config, ensemble->getL3Asics(), ensemble->isSai());

  if (hwAsic->isSupported(HwAsic::Feature::L3_QOS)) {
    addNetworkAIQosToConfig(config, hwSwitch);
  }
  if (hwAsic->isSupported(HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    addLoadBalancerToConfig(config, hwSwitch, LBHash::FULL_HASH);
  }

  setDefaultCpuTrafficPolicyConfig(
      config, std::vector<const HwAsic*>({hwAsic}), ensemble->isSai());
  if (hwSwitch->getPlatform()->getAsic()->isSupported(HwAsic::Feature::PFC)) {
    // pfc works reliably only in mmu lossless mode
    utility::addUplinkDownlinkPfcConfig(config, hwSwitch, uplinks, downlinks);
  }
  return config;
}

/*
 * Creates and returns a SwitchConfig which is as close to what you would find
 * in a production RSW as possible. If more features are desired, they can be
 * added in this function.
 *
 * Mainly to be called from HwProdInvariantsTest for config setup, and can be
 * used anywhere else it might be useful to have a prod RSW config.
 */
cfg::SwitchConfig createProdRswConfig(
    const std::vector<const HwAsic*>& asics,
    PlatformType platformType,
    const PlatformMapping* platformMapping,
    bool supportsAddRemovePort,
    const std::vector<PortID>& masterLogicalPortIds,
    bool isSai,
    bool enableStrictPriority) {
  auto hwAsic = checkSameAndGetAsic(asics);
  auto numUplinks = uplinksCountFromSwitch(platformType);

  // its the same speed used for the uplink and downlink for now
  auto uplinkSpeed = getPortSpeed(hwAsic->getAsicType(), platformType);
  auto downlinkSpeed = getPortSpeed(hwAsic->getAsicType(), platformType);

  // Create initial config to which we can add the rest of the features.
  auto config = createUplinkDownlinkConfig(
      platformMapping,
      hwAsic,
      platformType,
      supportsAddRemovePort,
      masterLogicalPortIds,
      numUplinks,
      uplinkSpeed,
      downlinkSpeed,
      hwAsic->desiredLoopbackModes());

  addCpuQueueConfig(config, asics, isSai);

  if (hwAsic->isSupported(HwAsic::Feature::L3_QOS)) {
    addOlympicQosToConfig(config, hwAsic, enableStrictPriority);
  }
  setDefaultCpuTrafficPolicyConfig(
      config, std::vector<const HwAsic*>({hwAsic}), isSai);
  if (hwAsic->isSupported(HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    addLoadBalancerToConfig(config, hwAsic, LBHash::FULL_HASH);
  }

  if (hwAsic->isSupported(HwAsic::Feature::MPLS) && !isSai) {
    addMplsConfig(config);
  }
  return config;
}

cfg::SwitchConfig createProdRswConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& masterLogicalPortIds,
    bool isSai,
    bool enableStrictPriority) {
  return createProdRswConfig(
      std::vector<const HwAsic*>({hwSwitch->getPlatform()->getAsic()}),
      hwSwitch->getPlatform()->getType(),
      hwSwitch->getPlatform()->getPlatformMapping(),
      hwSwitch->getPlatform()->supportsAddRemovePort(),
      masterLogicalPortIds,
      isSai,
      enableStrictPriority);
}
/*
 * Returns a prod-setting FSW config. Can be modified as desired for more
 * features to be added to config.
 */
cfg::SwitchConfig createProdFswConfig(
    const HwAsic* hwAsic,
    PlatformType platformType,
    const PlatformMapping* platformMapping,
    bool supportsAddRemovePort,
    const std::vector<PortID>& masterLogicalPortIds,
    bool isSai,
    bool enableStrictPriority) {
  auto numUplinks = uplinksCountFromSwitch(platformType);

  // its the same speed used for the uplink and downlink for now
  auto uplinkSpeed = getPortSpeed(hwAsic->getAsicType(), platformType);
  auto downlinkSpeed = getPortSpeed(hwAsic->getAsicType(), platformType);

  auto config = createUplinkDownlinkConfig(
      platformMapping,
      hwAsic,
      platformType,
      supportsAddRemovePort,
      masterLogicalPortIds,
      numUplinks,
      uplinkSpeed,
      downlinkSpeed,
      hwAsic->desiredLoopbackModes());

  addCpuQueueConfig(config, {hwAsic}, isSai);
  if (hwAsic->isSupported(HwAsic::Feature::L3_QOS)) {
    addOlympicQosToConfig(config, hwAsic, enableStrictPriority);
  }
  setDefaultCpuTrafficPolicyConfig(
      config, std::vector<const HwAsic*>({hwAsic}), isSai);
  if (hwAsic->isSupported(HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    addLoadBalancerToConfig(config, hwAsic, LBHash::HALF_HASH);
  }
  if (hwAsic->isSupported(HwAsic::Feature::MPLS) && !isSai) {
    addMplsConfig(config);
  }
  return config;
}

cfg::SwitchConfig createProdFswConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& masterLogicalPortIds,
    bool isSai,
    bool enableStrictPriority) {
  return createProdFswConfig(
      hwSwitch->getPlatform()->getAsic(),
      hwSwitch->getPlatform()->getType(),
      hwSwitch->getPlatform()->getPlatformMapping(),
      hwSwitch->getPlatform()->supportsAddRemovePort(),
      masterLogicalPortIds,
      isSai,
      enableStrictPriority);
}

cfg::SwitchConfig createProdRswMhnicConfig(
    const HwAsic* hwAsic,
    PlatformType platformType,
    const PlatformMapping* platformMapping,
    bool supportsAddRemovePort,
    const std::vector<PortID>& masterLogicalPortIds,
    bool isSai) {
  auto numUplinks = uplinksCountFromSwitch(platformType);
  auto uplinkSpeed = getPortSpeed(hwAsic->getAsicType(), platformType);
  auto downlinkSpeed = getPortSpeed(hwAsic->getAsicType(), platformType);

  auto config = createUplinkDownlinkConfig(
      platformMapping,
      hwAsic,
      platformType,
      supportsAddRemovePort,
      masterLogicalPortIds,
      numUplinks,
      uplinkSpeed,
      downlinkSpeed,
      hwAsic->desiredLoopbackModes());

  addCpuQueueConfig(config, {hwAsic}, isSai);
  setDefaultCpuTrafficPolicyConfig(
      config, std::vector<const HwAsic*>({hwAsic}), isSai);
  if (hwAsic->isSupported(HwAsic::Feature::L3_QOS)) {
    addQueuePerHostToConfig(config, isSai);
    // DSCP Marking ACLs must be programmed AFTER queue-per-host ACLs or else
    // traffic matching DSCP Marking ACLs will only hit DSCP Marking ACLs and
    // thus suffer from noisy neighbor.
    // The queue-per-host ACLs match on traffic to downlinks.
    // Thus, putting DSCP Marking ACLs before queue-per-host ACLs would cause
    // noisy neighbor problem for traffic between ports connected to the same
    // switch.
    utility::addDscpMarkingAcls(hwAsic, &config, isSai);
  }
  if (hwAsic->isSupported(HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    addLoadBalancerToConfig(config, hwAsic, LBHash::FULL_HASH);
  }
  if (hwAsic->isSupported(HwAsic::Feature::MPLS) && !isSai) {
    addMplsConfig(config);
  }
  return config;
}

cfg::SwitchConfig createProdRswMhnicConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& masterLogicalPortIds,
    bool isSai) {
  return createProdRswMhnicConfig(
      hwSwitch->getPlatform()->getAsic(),
      hwSwitch->getPlatform()->getType(),
      hwSwitch->getPlatform()->getPlatformMapping(),
      hwSwitch->getPlatform()->supportsAddRemovePort(),
      masterLogicalPortIds,
      isSai);
}
} // namespace facebook::fboss::utility
