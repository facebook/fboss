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
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"

#include "fboss/agent/test/utils/OlympicTestUtils.h"

namespace facebook::fboss::utility {

void addProdFeaturesToConfig(
    cfg::SwitchConfig& config,
    const HwAsic* hwAsic,
    bool isSai) {
  /*
   * Configures port queue for cpu port
   */
  utility::addCpuQueueConfig(config, {hwAsic}, isSai);
  if (hwAsic->isSupported(HwAsic::Feature::L3_QOS)) {
    /*
     * Enable Olympic QOS
     */
    utility::addOlympicQosMaps(config, {hwAsic});

    /*
     * Enable Olympic Queue Config
     */
    utility::addOlympicQueueConfig(&config, {hwAsic}, true);
  }
  /*
   * Configure COPP, CPU traffic policy and ACLs
   */
  utility::setDefaultCpuTrafficPolicyConfig(
      config, std::vector<const HwAsic*>({hwAsic}), isSai);

  /*
   * Enable Load balancer
   */
  if (hwAsic->isSupported(HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    config.loadBalancers()->push_back(utility::getEcmpFullHashConfig({hwAsic}));
  }
}
} // namespace facebook::fboss::utility
