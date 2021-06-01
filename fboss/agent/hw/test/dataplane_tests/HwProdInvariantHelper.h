/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"

#include "fboss/agent/hw/test/HwTestProdConfigUtils.h"

#include "fboss/agent/hw/test/dataplane_tests/HwEcmpDataPlaneTestUtil.h"

namespace facebook::fboss {

class HwProdInvariantHelper {
 public:
  HwProdInvariantHelper(
      HwSwitchEnsemble* ensemble,
      const cfg::SwitchConfig& initialCfg)
      : ensemble_(ensemble), initialCfg_(initialCfg) {}

  void setupEcmp();
  void setupEcmpWithNextHopMac(const folly::MacAddress& nextHop);
  void verifyInvariants() {
    verifySafeDiagCmds();
    verifyDscpToQueueMapping();
    verifyCopp();
    verifyLoadBalacing();
  }
  static HwSwitchEnsemble::Features featuresDesired() {
    return {
        HwSwitchEnsemble::LINKSCAN,
        HwSwitchEnsemble::PACKET_RX,
        HwSwitchEnsemble::STATS_COLLECTION};
  }
  void verifyNoDiscards();

 private:
  cfg::SwitchConfig initialConfig() const {
    return initialCfg_;
  }
  std::vector<std::string> getBcmCommandsToTest() const;
  void verifySafeDiagCmds();
  void verifyDscpToQueueMapping();
  void verifyLoadBalacing();
  void verifyCopp();

  std::unique_ptr<utility::HwIpV6EcmpDataPlaneTestUtil> ecmpHelper_;
  HwSwitchEnsemble* ensemble_;
  cfg::SwitchConfig initialCfg_;
};

} // namespace facebook::fboss
