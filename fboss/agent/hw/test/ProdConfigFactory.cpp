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
#include "fboss/agent/hw/test/dataplane_tests/HwTestDscpMarkingUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestPfcUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQueuePerHostUtils.h"

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
    const HwSwitch* hwSwitch) {
  auto hwAsic = hwSwitch->getPlatform()->getAsic();
  addOlympicQosMaps(config, hwAsic);
  auto streamType =
      *hwAsic->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT).begin();
  addOlympicQueueConfig(&config, streamType, hwAsic);
}

void addNetworkAIQosToConfig(
    cfg::SwitchConfig& config,
    const HwSwitch* hwSwitch) {
  auto hwAsic = hwSwitch->getPlatform()->getAsic();
  // network AI qos map is the same as olympic
  addOlympicQosMaps(config, hwAsic);
  auto streamType =
      *hwAsic->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT).begin();
  // queue configuration is different
  addNetworkAIQueueConfig(&config, streamType, hwAsic);
}

/*
 * Enable load balancing on provided config. Uses an enum, declared in
 * ConfigFactory.h, to decide whether to apply full-hash or half-hash config.
 * These are the only two current hashing algorithms at the time of writing; if
 * any more are added, this function and the enum can be modified accordingly.
 */
void addLoadBalancerToConfig(
    cfg::SwitchConfig& config,
    const HwSwitch* hwSwitch,
    LBHash hashType) {
  auto platform = hwSwitch->getPlatform();
  auto hwAsic = platform->getAsic();
  if (!hwAsic->isSupported(HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    return;
  }
  switch (hashType) {
    case LBHash::FULL_HASH:
      config.loadBalancers()->push_back(getEcmpFullHashConfig(platform));
      break;
    case LBHash::HALF_HASH:
      config.loadBalancers()->push_back(getEcmpHalfHashConfig(platform));
      break;
    default:
      throw FbossError("invalid hashing option ", hashType);
      break;
  }
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
uint16_t uplinksCountFromSwitch(const HwSwitch* hwSwitch) {
  auto mode = hwSwitch->getPlatform()->getType();
  using PM = PlatformType;
  switch (mode) {
    case PM::PLATFORM_WEDGE:
    case PM::PLATFORM_WEDGE100:
    case PM::PLATFORM_WEDGE400C:
    case PM::PLATFORM_WEDGE400:
    case PM::PLATFORM_YAMP:
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

cfg::PortSpeed getPortSpeed(const HwSwitch* hwSwitch) {
  auto hwAsicType = hwSwitch->getPlatform()->getAsic()->getAsicType();
  auto platformType = hwSwitch->getPlatform()->getType();
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

/*
 * Adds queue-per-host mapping to config, based on HwQueuePerHostRouteTests.cpp.
 * Other helper functions (i.e. addRoutes, updateRoutesClassID) are called on
 * the test fixture's setup() phase.
 */
void addQueuePerHostToConfig(cfg::SwitchConfig& config) {
  utility::addQueuePerHostQueueConfig(&config);
  utility::addQueuePerHostAcls(&config);
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
      hwAsic->desiredLoopbackMode(),
      uplinks,
      downlinks);

  addCpuQueueConfig(config, hwAsic);

  if (hwAsic->isSupported(HwAsic::Feature::L3_QOS)) {
    addNetworkAIQosToConfig(config, hwSwitch);
  }
  if (hwAsic->isSupported(HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    addLoadBalancerToConfig(config, hwSwitch, LBHash::FULL_HASH);
  }

  setDefaultCpuTrafficPolicyConfig(config, hwAsic);
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
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& masterLogicalPortIds,
    bool isSai) {
  auto platform = hwSwitch->getPlatform();
  auto hwAsic = platform->getAsic();

  auto numUplinks = uplinksCountFromSwitch(hwSwitch);

  // its the same speed used for the uplink and downlink for now
  auto uplinkSpeed = getPortSpeed(hwSwitch);
  auto downlinkSpeed = getPortSpeed(hwSwitch);

  // Create initial config to which we can add the rest of the features.
  auto config = createUplinkDownlinkConfig(
      hwSwitch,
      masterLogicalPortIds,
      numUplinks,
      uplinkSpeed,
      downlinkSpeed,
      hwAsic->desiredLoopbackMode());

  addCpuQueueConfig(config, hwAsic);

  if (hwAsic->isSupported(HwAsic::Feature::L3_QOS)) {
    addOlympicQosToConfig(config, hwSwitch);
  }
  setDefaultCpuTrafficPolicyConfig(config, hwAsic);
  if (hwAsic->isSupported(HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    addLoadBalancerToConfig(config, hwSwitch, LBHash::FULL_HASH);
  }

  if (hwAsic->isSupported(HwAsic::Feature::MPLS) && !isSai) {
    addMplsConfig(config);
  }
  return config;
}

/*
 * Returns a prod-setting FSW config. Can be modified as desired for more
 * features to be added to config.
 */
cfg::SwitchConfig createProdFswConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& masterLogicalPortIds,
    bool isSai) {
  auto platform = hwSwitch->getPlatform();
  auto hwAsic = platform->getAsic();

  auto numUplinks = uplinksCountFromSwitch(hwSwitch);

  // its the same speed used for the uplink and downlink for now
  auto uplinkSpeed = getPortSpeed(hwSwitch);
  auto downlinkSpeed = getPortSpeed(hwSwitch);

  auto config = createUplinkDownlinkConfig(
      hwSwitch,
      masterLogicalPortIds,
      numUplinks,
      uplinkSpeed,
      downlinkSpeed,
      hwAsic->desiredLoopbackMode());

  addCpuQueueConfig(config, hwAsic);
  if (hwAsic->isSupported(HwAsic::Feature::L3_QOS)) {
    addOlympicQosToConfig(config, hwSwitch);
  }
  setDefaultCpuTrafficPolicyConfig(config, hwAsic);
  if (hwAsic->isSupported(HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    addLoadBalancerToConfig(config, hwSwitch, LBHash::HALF_HASH);
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
  auto platform = hwSwitch->getPlatform();
  auto hwAsic = platform->getAsic();

  auto numUplinks = uplinksCountFromSwitch(hwSwitch);
  auto uplinkSpeed = getPortSpeed(hwSwitch);
  auto downlinkSpeed = getPortSpeed(hwSwitch);

  auto config = createUplinkDownlinkConfig(
      hwSwitch,
      masterLogicalPortIds,
      numUplinks,
      uplinkSpeed,
      downlinkSpeed,
      hwAsic->desiredLoopbackMode());

  addCpuQueueConfig(config, hwAsic);
  setDefaultCpuTrafficPolicyConfig(config, hwAsic);
  if (hwAsic->isSupported(HwAsic::Feature::L3_QOS)) {
    addQueuePerHostToConfig(config);
    // DSCP Marking ACLs must be programmed AFTER queue-per-host ACLs or else
    // traffic matching DSCP Marking ACLs will only hit DSCP Marking ACLs and
    // thus suffer from noisy neighbor.
    // The queue-per-host ACLs match on traffic to downlinks.
    // Thus, putting DSCP Marking ACLs before queue-per-host ACLs would cause
    // noisy neighbor problem for traffic between ports connected to the same
    // switch.
    utility::addDscpMarkingAcls(&config, hwAsic);
  }
  if (hwAsic->isSupported(HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    addLoadBalancerToConfig(config, hwSwitch, LBHash::FULL_HASH);
  }
  if (hwAsic->isSupported(HwAsic::Feature::MPLS) && !isSai) {
    addMplsConfig(config);
  }
  return config;
}

} // namespace facebook::fboss::utility
