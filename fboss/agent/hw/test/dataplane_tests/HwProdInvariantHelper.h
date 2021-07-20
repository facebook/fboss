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

/*
 * Defines an enum to pass test options to functions - specifically, to check
 * which features a given switch has enabled.
 * For a function that takes such options, define an argument of type
 * HwInvariantBitmask which can then be bit-masked with different HwInvariantOpt
 * values.
 */
typedef uint32_t HwInvariantBitmask;
enum HwInvariantOpt : HwInvariantBitmask {
  OPT_NONE = 0x0000,
  OPT_MMU_LOSSLESS = 0x0001,
  OPT_MHNIC = 0x0002,
  OPT_SFLOW = 0x0004,
  OPT_COPP = 0x0008,
  OPT_LOAD_BALANCER = 0x0010,
  OPT_OLYMPIC_QOS = 0x0020,
};

/*
 * Convenience function that checks if field `opt` is set within `opts`.
 * Useful to avoid human error in checking bitmasks.
 */
inline bool isHwInvariantOptSet(HwInvariantBitmask opts, HwInvariantOpt opt) {
  return ((opts & opt) == opt);
}

class HwProdInvariantHelper {
 public:
  HwProdInvariantHelper(
      HwSwitchEnsemble* ensemble,
      const cfg::SwitchConfig& initialCfg)
      : ensemble_(ensemble), initialCfg_(initialCfg) {}

  void setupEcmp();
  void setupEcmpWithNextHopMac(const folly::MacAddress& nextHop);
  void disableTtl();
  void verifyInvariants() {
    verifySafeDiagCmds();
    verifyDscpToQueueMapping();
    verifyCopp();
    verifyLoadBalacing();
  }
  void sendTraffic();
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
  void verifyCopp();
  void verifyLoadBalacing();
  void sendAndVerifyPkts(uint16_t destPort, uint8_t queueId);

  std::unique_ptr<utility::HwIpV6EcmpDataPlaneTestUtil> ecmpHelper_;
  HwSwitchEnsemble* ensemble_;
  cfg::SwitchConfig initialCfg_;
};

} // namespace facebook::fboss
