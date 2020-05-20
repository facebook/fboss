// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/dataplane_tests/HwEcmpDataPlaneTestUtil.h"

#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateToggler.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

namespace facebook::fboss::utility {

template <typename EcmpSetupHelperT>
void HwEcmpDataPlaneTestUtil<EcmpSetupHelperT>::pumpTraffic(
    int ecmpWidth,
    bool loopThroughFrontPanel) {
  std::optional<PortID> frontPanelPortToLoopTraffic;
  if (loopThroughFrontPanel) {
    frontPanelPortToLoopTraffic =
        helper_->ecmpPortDescriptorAt(ecmpWidth).phyPortID();
  }
  pumpTrafficThroughPort(frontPanelPortToLoopTraffic);
}

template <typename EcmpSetupHelperT>
void HwEcmpDataPlaneTestUtil<EcmpSetupHelperT>::unresolveNextHop(
    unsigned int id) {
  auto portDesc = helper_->ecmpPortDescriptorAt(id);
  ensemble_->applyNewState(
      helper_->unresolveNextHops(ensemble_->getProgrammedState(), {portDesc}));
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

template <typename AddrT>
HwIpEcmpDataPlaneTestUtil<AddrT>::HwIpEcmpDataPlaneTestUtil(
    HwSwitchEnsemble* ensemble,
    RouterID vrf,
    std::vector<LabelForwardingAction::LabelStack> stacks)
    : BaseT(
          ensemble,
          std::make_unique<EcmpSetupAnyNPortsT>(
              ensemble->getProgrammedState(),
              vrf)),
      stacks_(std::move(stacks)) {}

template <typename AddrT>
HwIpEcmpDataPlaneTestUtil<AddrT>::HwIpEcmpDataPlaneTestUtil(
    HwSwitchEnsemble* ensemble,
    RouterID vrf)
    : HwIpEcmpDataPlaneTestUtil(ensemble, vrf, {}) {}

template <typename AddrT>
void HwIpEcmpDataPlaneTestUtil<AddrT>::setupECMPForwarding(
    int ecmpWidth,
    const std::vector<NextHopWeight>& weights) {
  auto* helper = BaseT::ecmpSetupHelper();
  auto* ensemble = BaseT::getEnsemble();
  auto state =
      helper->resolveNextHops(ensemble->getProgrammedState(), ecmpWidth);
  auto newState = stacks_.empty()
      ? helper->setupECMPForwarding(state, ecmpWidth, {{AddrT(), 0}}, weights)
      : helper->setupIp2MplsECMPForwarding(
            state, ecmpWidth, {{AddrT(), 0}}, stacks_, weights);
  ensemble->applyNewState(newState);
}

template <typename AddrT>
void HwIpEcmpDataPlaneTestUtil<AddrT>::pumpTrafficThroughPort(
    std::optional<PortID> port) {
  auto* ensemble = BaseT::getEnsemble();
  auto programmedState = ensemble->getProgrammedState();
  auto firstVlan = *(programmedState->getVlans()->begin());
  auto mac = utility::getInterfaceMac(programmedState, firstVlan->getID());

  utility::pumpTraffic(
      std::is_same_v<AddrT, folly::IPAddressV6>,
      ensemble->getHwSwitch(),
      mac,
      firstVlan->getID(),
      port);
}

template <typename AddrT>
HwMplsEcmpDataPlaneTestUtil<AddrT>::HwMplsEcmpDataPlaneTestUtil(
    HwSwitchEnsemble* ensemble,
    MPLSHdr::Label topLabel,
    LabelForwardingAction::LabelForwardingType actionType)
    : BaseT(
          ensemble,
          std::make_unique<EcmpSetupAnyNPortsT>(
              ensemble->getProgrammedState(),
              topLabel.getLabelValue(),
              actionType)),
      label_(topLabel) {}

template <typename AddrT>
void HwMplsEcmpDataPlaneTestUtil<AddrT>::setupECMPForwarding(
    int ecmpWidth,
    const std::vector<NextHopWeight>& weights) {
  auto* helper = BaseT::ecmpSetupHelper();
  auto* ensemble = BaseT::getEnsemble();
  auto state =
      helper->resolveNextHops(ensemble->getProgrammedState(), ecmpWidth);
  auto newState = helper->setupECMPForwarding(state, ecmpWidth, weights);
  ensemble->applyNewState(newState);
}

template <typename AddrT>
void HwMplsEcmpDataPlaneTestUtil<AddrT>::pumpTrafficThroughPort(
    std::optional<PortID> port) {
  /* pump MPLS traffic */
  auto* ensemble = BaseT::getEnsemble();
  auto programmedState = ensemble->getProgrammedState();
  auto firstVlan = *(programmedState->getVlans()->begin());
  auto mac = utility::getInterfaceMac(programmedState, firstVlan->getID());
  pumpMplsTraffic(
      std::is_same_v<AddrT, folly::IPAddressV6>,
      ensemble->getHwSwitch(),
      label_.getLabelValue(),
      mac,
      port);
}

template class HwEcmpDataPlaneTestUtil<utility::EcmpSetupAnyNPorts4>;
template class HwEcmpDataPlaneTestUtil<utility::EcmpSetupAnyNPorts6>;
template class HwEcmpDataPlaneTestUtil<
    utility::MplsEcmpSetupAnyNPorts<folly::IPAddressV4>>;
template class HwEcmpDataPlaneTestUtil<
    utility::MplsEcmpSetupAnyNPorts<folly::IPAddressV6>>;
template class HwIpEcmpDataPlaneTestUtil<folly::IPAddressV4>;
template class HwIpEcmpDataPlaneTestUtil<folly::IPAddressV6>;
template class HwMplsEcmpDataPlaneTestUtil<folly::IPAddressV4>;
template class HwMplsEcmpDataPlaneTestUtil<folly::IPAddressV6>;

} // namespace facebook::fboss::utility
