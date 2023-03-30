/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestProdConfigUtils.h"

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestPfcUtils.h"

namespace facebook::fboss::utility {

void addProdFeaturesToConfig(
    cfg::SwitchConfig& config,
    const HwSwitch* hwSwitch,
    bool mmuLossless,
    const std::vector<PortID>& ports) {
  auto hwAsic = hwSwitch->getPlatform()->getAsic();
  /*
   * Configures port queue for cpu port
   */
  utility::addCpuQueueConfig(config, hwAsic);
  if (hwAsic->isSupported(HwAsic::Feature::L3_QOS)) {
    /*
     * Enable Olympic QOS
     */
    utility::addOlympicQosMaps(config, hwAsic);

    /*
     * Enable Olympic Queue Config
     */
    auto streamType = *(hwSwitch->getPlatform()
                            ->getAsic()
                            ->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT)
                            .begin());
    utility::addOlympicQueueConfig(&config, streamType, hwAsic, true);
  }
  /*
   * Configure COPP, CPU traffic policy and ACLs
   */
  utility::setDefaultCpuTrafficPolicyConfig(config, hwAsic);

  /*
   * Enable Load balancer
   */
  if (hwAsic->isSupported(HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    config.loadBalancers()->push_back(
        utility::getEcmpFullHashConfig(hwSwitch->getPlatform()));
  }

  if (hwAsic->isSupported(HwAsic::Feature::PFC) && mmuLossless) {
    // pfc works reliably only in mmu lossless mode
    utility::addPfcConfig(config, hwSwitch, ports);
  }
}
} // namespace facebook::fboss::utility
