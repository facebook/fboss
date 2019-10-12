// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/dataplane_tests/HwEcmpDataPlaneTestUtil.h"

#include "fboss/agent/hw/test/HwLinkStateToggler.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

namespace facebook {
namespace fboss {
namespace utility {

template <typename EcmpSetupHelperT>
void HwEcmpDataPlaneTestUtil<EcmpSetupHelperT>::pumpTraffic(
    int ecmpWidth,
    bool loopThroughFrontPanel) {
  folly::Optional<PortID> frontPanelPortToLoopTraffic;
  if (loopThroughFrontPanel) {
    frontPanelPortToLoopTraffic =
        helper_->ecmpPortDescriptorAt(ecmpWidth).phyPortID();
  }
  pumpTraffic(frontPanelPortToLoopTraffic);
}

template <typename EcmpSetupHelperT>
void HwEcmpDataPlaneTestUtil<EcmpSetupHelperT>::resolveNextHopsandClearStats(
    unsigned int ecmpWidth) {
  ensemble_->applyNewState(
      helper_->resolveNextHops(ensemble_->getProgrammedState(), ecmpWidth));

  while (ecmpWidth) {
    ecmpWidth--;
    auto ports = {
        static_cast<int32_t>(helper_->nhop(ecmpWidth).portDesc.phyPortID())};
    ensemble_->getHwSwitch()->clearPortStats(
        std::make_unique<std::vector<int32_t>>(ports));
  }
}

template <typename EcmpSetupHelperT>
void HwEcmpDataPlaneTestUtil<EcmpSetupHelperT>::shrinkECMP(
    unsigned int ecmpWidth) {
  std::vector<PortID> ports = {helper_->nhop(ecmpWidth).portDesc.phyPortID()};
  ensemble_->getLinkToggler()->bringDownPorts(
      ensemble_->getProgrammedState(), ports);
}

template <typename EcmpSetupHelperT>
void HwEcmpDataPlaneTestUtil<EcmpSetupHelperT>::expandECMP(
    unsigned int ecmpWidth) {
  std::vector<PortID> ports = {helper_->nhop(ecmpWidth).portDesc.phyPortID()};
  ensemble_->getLinkToggler()->bringUpPorts(
      ensemble_->getProgrammedState(), ports);
}

template <typename EcmpSetupHelperT>
bool HwEcmpDataPlaneTestUtil<EcmpSetupHelperT>::isLoadBalanced(
    int ecmpWidth,
    const std::vector<NextHopWeight>& weights,
    uint8_t deviation) {
  auto ecmpPorts = helper_->ecmpPortDescs(ecmpWidth);
  return utility::isLoadBalanced(ensemble_, ecmpPorts, weights, deviation);
}

template class HwEcmpDataPlaneTestUtil<utility::EcmpSetupAnyNPorts4>;
template class HwEcmpDataPlaneTestUtil<utility::EcmpSetupAnyNPorts6>;
template class HwEcmpDataPlaneTestUtil<
    utility::MplsEcmpSetupAnyNPorts<folly::IPAddressV4>>;
template class HwEcmpDataPlaneTestUtil<
    utility::MplsEcmpSetupAnyNPorts<folly::IPAddressV6>>;
} // namespace utility
} // namespace fboss
} // namespace facebook
