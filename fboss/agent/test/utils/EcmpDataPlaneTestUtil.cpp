// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/test/utils/EcmpDataPlaneTestUtil.h"

#include "fboss/agent/RouteUpdateWrapper.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/LinkStateToggler.h"
#include "fboss/agent/test/TestEnsembleIf.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"

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
  auto state = ensemble_->getProgrammedState();
  ensemble_->applyNewState(
      [=, this](const std::shared_ptr<SwitchState>& state) {
        return helper_->unresolveNextHops(state, {portDesc});
      },
      "unresolve-nexthops");
}

template <typename EcmpSetupHelperT>
void HwEcmpDataPlaneTestUtil<EcmpSetupHelperT>::resolveNextHopsandClearStats(
    unsigned int ecmpWidth) {
  ensemble_->applyNewState(
      [=, this](const std::shared_ptr<SwitchState>& state) {
        return helper_->resolveNextHops(state, ecmpWidth);
      },
      "resolve-nexthops-clear-stats");

  while (ecmpWidth) {
    ecmpWidth--;
    auto ports = {
        static_cast<int32_t>(helper_->nhop(ecmpWidth).portDesc.phyPortID())};
    ensemble_->clearPortStats(std::make_unique<std::vector<int32_t>>(ports));
  }
}

template <typename EcmpSetupHelperT>
void HwEcmpDataPlaneTestUtil<EcmpSetupHelperT>::shrinkECMP(
    unsigned int ecmpWidth,
    bool clearStats) {
  std::vector<PortID> ports = {helper_->nhop(ecmpWidth).portDesc.phyPortID()};
  ensemble_->getLinkToggler()->bringDownPorts(ports);
  if (clearStats) {
    resolveNextHopsandClearStats(ecmpWidth);
  }
}

template <typename EcmpSetupHelperT>
void HwEcmpDataPlaneTestUtil<EcmpSetupHelperT>::expandECMP(
    unsigned int ecmpWidth,
    bool clearStats) {
  std::vector<PortID> ports = {helper_->nhop(ecmpWidth).portDesc.phyPortID()};
  ensemble_->getLinkToggler()->bringUpPorts(ports);
  if (clearStats) {
    resolveNextHopsandClearStats(ecmpWidth);
  }
}

template <typename EcmpSetupHelperT>
bool HwEcmpDataPlaneTestUtil<EcmpSetupHelperT>::isLoadBalanced(
    int ecmpWidth,
    const std::vector<NextHopWeight>& weights,
    uint8_t deviation) {
  std::vector<PortDescriptor> ecmpPorts = helper_->ecmpPortDescs(ecmpWidth);
  return isLoadBalanced(ecmpPorts, weights, deviation);
}

template <typename EcmpSetupHelperT>
bool HwEcmpDataPlaneTestUtil<EcmpSetupHelperT>::isLoadBalanced(
    const std::vector<PortDescriptor>& portDescs,
    const std::vector<NextHopWeight>& weights,
    uint8_t deviation) {
  auto rc = utility::isLoadBalanced<PortID, HwPortStats>(
      portDescs,
      weights,
      [ensemble = ensemble_](
          const std::vector<PortID>& portIds) -> std::map<PortID, HwPortStats> {
        return ensemble->getLatestPortStats(portIds);
      },
      deviation);
  return rc;
}

template <typename AddrT>
void HwEcmpDataPlaneTestUtil<AddrT>::programLoadBalancer(
    const cfg::LoadBalancer& lb) {
  if (ensemble_->getHwAsicTable()
          ->getSwitchIDs(HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)
          .empty()) {
    return;
  }
  auto config = ensemble_->getCurrentConfig();
  config.loadBalancers()->clear();
  config.loadBalancers()->push_back(lb);
  if (ensemble_->isSai()) {
    // always program half lag hash for sai switches, see CS00012317640
    auto switchIds = ensemble_->getHwAsicTable()->getSwitchIDs(
        HwAsic::Feature::SAI_LAG_HASH);
    if (!switchIds.empty()) {
      config.loadBalancers()->push_back(
          utility::getTrunkHalfHashConfig(ensemble_->getL3Asics()));
    }
  }
  XLOG(INFO) << "Programming load balancer: " << *lb.id();
  ensemble_->applyNewConfig(config);
}

template <typename AddrT>
void HwEcmpDataPlaneTestUtil<AddrT>::pumpTrafficPortAndVerifyLoadBalanced(
    unsigned int ecmpWidth,
    bool loopThroughFrontPanel,
    const std::vector<NextHopWeight>& weights,
    int deviation,
    bool loadBalanceExpected) {
  utility::pumpTrafficAndVerifyLoadBalanced(
      [=, this]() { this->pumpTraffic(ecmpWidth, loopThroughFrontPanel); },
      [=, this]() {
        auto helper = this->ecmpSetupHelper();
        auto portDescs = helper->getPortDescs(ecmpWidth);
        auto ports = std::make_unique<std::vector<int32_t>>();
        for (const auto& portDesc : portDescs) {
          if (portDesc.isPhysicalPort()) {
            ports->push_back(portDesc.phyPortID());
          }
        }
        ensemble_->clearPortStats(ports);
      },
      [=, this]() {
        return this->isLoadBalanced(ecmpWidth, weights, deviation);
      },
      loadBalanceExpected);
}

template <typename AddrT>
HwIpEcmpDataPlaneTestUtil<AddrT>::HwIpEcmpDataPlaneTestUtil(
    TestEnsembleIf* ensemble,
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
    TestEnsembleIf* ensemble,
    const std::optional<folly::MacAddress>& nextHopMac,
    RouterID vrf)
    : BaseT(
          ensemble,
          std::make_unique<EcmpSetupAnyNPortsT>(
              ensemble->getProgrammedState(),
              nextHopMac,
              vrf)) {}

template <typename AddrT>
HwIpEcmpDataPlaneTestUtil<AddrT>::HwIpEcmpDataPlaneTestUtil(
    TestEnsembleIf* ensemble,
    RouterID vrf)
    : HwIpEcmpDataPlaneTestUtil(ensemble, vrf, {}) {}

template <typename AddrT>
void HwIpEcmpDataPlaneTestUtil<AddrT>::programRoutes(
    int ecmpWidth,
    const std::vector<NextHopWeight>& weights) {
  auto* helper = BaseT::ecmpSetupHelper();
  auto* ensemble = BaseT::getEnsemble();
  ensemble->applyNewState([=, this](const std::shared_ptr<SwitchState>& state) {
    return helper->resolveNextHops(state, ecmpWidth);
  });
  if (stacks_.empty()) {
    helper->programRoutes(
        ensemble->getRouteUpdaterWrapper(), ecmpWidth, {{AddrT(), 0}}, weights);
  } else {
    helper->programIp2MplsRoutes(
        ensemble->getRouteUpdaterWrapper(),
        ecmpWidth,
        {{AddrT(), 0}},
        stacks_,
        weights);
  }
}

template <typename AddrT>
void HwIpEcmpDataPlaneTestUtil<AddrT>::programRoutes(
    const boost::container::flat_set<PortDescriptor>& portDescs,
    const std::vector<NextHopWeight>& weights) {
  auto* helper = BaseT::ecmpSetupHelper();
  auto* ensemble = BaseT::getEnsemble();
  ensemble->applyNewState([=, this](const std::shared_ptr<SwitchState>& state) {
    return helper->resolveNextHops(state, portDescs);
  });

  // TODO: add a stacks_.empty() check, i.e. lines 128-130
  helper->programRoutes(
      ensemble->getRouteUpdaterWrapper(), portDescs, {{AddrT(), 0}}, weights);
}

/*
 * This is nothing more than a convenience wrapper function around the above
 * programRoutes that takes in a flat_set of PortDescriptors.
 *
 * In this way, we can call programRoutes (e.g. from ProdInvariantHelper) with
 * just the vector of port descriptors to set up.
 */
template <typename AddrT>
void HwIpEcmpDataPlaneTestUtil<AddrT>::programRoutesVecHelper(
    const std::vector<PortDescriptor>& portDescs,
    const std::vector<NextHopWeight>& weights) {
  boost::container::flat_set<PortDescriptor> fsDescs;
  for (auto desc : portDescs) {
    fsDescs.insert(desc);
  }
  programRoutes(fsDescs, weights);
}

template <typename AddrT>
const std::vector<EcmpNextHop<AddrT>>
HwIpEcmpDataPlaneTestUtil<AddrT>::getNextHops() {
  auto* helper = BaseT::ecmpSetupHelper();
  return helper->getNextHops();
}

template <typename AddrT>
void HwIpEcmpDataPlaneTestUtil<AddrT>::pumpTrafficThroughPort(
    std::optional<PortID> port) {
  auto* ensemble = BaseT::getEnsemble();
  auto programmedState = ensemble->getProgrammedState();
  auto vlanId = utility::firstVlanID(programmedState);
  auto intfMac = utility::getFirstInterfaceMac(programmedState);

  utility::pumpTraffic(
      std::is_same_v<AddrT, folly::IPAddressV6>,
      utility::getAllocatePktFn(ensemble),
      utility::getSendPktFunc(ensemble),
      intfMac,
      vlanId,
      port);
}

template <typename AddrT>
HwIpRoCEEcmpDataPlaneTestUtil<AddrT>::HwIpRoCEEcmpDataPlaneTestUtil(
    TestEnsembleIf* ensemble,
    RouterID vrf)
    : BaseT(ensemble, vrf) {}

template <typename AddrT>
void HwIpRoCEEcmpDataPlaneTestUtil<AddrT>::pumpTrafficThroughPort(
    std::optional<PortID> port) {
  auto* ensemble = BaseT::getEnsemble();
  auto programmedState = ensemble->getProgrammedState();
  auto vlanId = utility::firstVlanID(programmedState);
  auto intfMac = utility::getFirstInterfaceMac(programmedState);

  utility::pumpRoCETraffic(
      std::is_same_v<AddrT, folly::IPAddressV6>,
      utility::getAllocatePktFn(ensemble),
      utility::getSendPktFunc(ensemble),
      intfMac,
      vlanId,
      port);
}

template <typename AddrT>
HwIpRoCEEcmpDestPortDataPlaneTestUtil<AddrT>::
    HwIpRoCEEcmpDestPortDataPlaneTestUtil(
        TestEnsembleIf* ensemble,
        RouterID vrf)
    : BaseT(ensemble, vrf) {}

template <typename AddrT>
void HwIpRoCEEcmpDestPortDataPlaneTestUtil<AddrT>::pumpTrafficThroughPort(
    std::optional<PortID> port) {
  auto* ensemble = BaseT::getEnsemble();
  auto programmedState = ensemble->getProgrammedState();
  auto vlanId = utility::firstVlanID(programmedState);
  auto intfMac = utility::getFirstInterfaceMac(programmedState);

  utility::pumpRoCETraffic(
      std::is_same_v<AddrT, folly::IPAddressV6>,
      utility::getAllocatePktFn(ensemble),
      utility::getSendPktFunc(ensemble),
      intfMac,
      vlanId,
      port,
      kRandomUdfL4SrcPort);
}

template <typename AddrT>
HwMplsEcmpDataPlaneTestUtil<AddrT>::HwMplsEcmpDataPlaneTestUtil(
    TestEnsembleIf* ensemble,
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
void HwMplsEcmpDataPlaneTestUtil<AddrT>::programRoutes(
    int ecmpWidth,
    const std::vector<NextHopWeight>& weights) {
  auto* helper = BaseT::ecmpSetupHelper();
  auto* ensemble = BaseT::getEnsemble();
  ensemble->applyNewState(
      [=, this](const std::shared_ptr<SwitchState>& state) {
        return helper->resolveNextHops(state, ecmpWidth);
      },
      "resolve next hops");
  helper->setupECMPForwarding(
      ensemble->getProgrammedState(),
      ensemble->getRouteUpdaterWrapper(),
      ecmpWidth,
      weights);
}

template <typename AddrT>
void HwMplsEcmpDataPlaneTestUtil<AddrT>::pumpTrafficThroughPort(
    std::optional<PortID> port) {
  /* pump MPLS traffic */
  auto* ensemble = BaseT::getEnsemble();
  auto programmedState = ensemble->getProgrammedState();
  auto firstVlanID = programmedState->getVlans()->getFirstVlanID();
  auto mac = utility::getInterfaceMac(programmedState, firstVlanID);

  pumpMplsTraffic(
      std::is_same_v<AddrT, folly::IPAddressV6>,
      utility::getAllocatePktFn(ensemble),
      utility::getSendPktFunc(ensemble),
      label_.getLabelValue(),
      mac,
      firstVlanID,
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
template class HwIpRoCEEcmpDataPlaneTestUtil<folly::IPAddressV6>;
template class HwIpRoCEEcmpDataPlaneTestUtil<folly::IPAddressV4>;
template class HwIpRoCEEcmpDestPortDataPlaneTestUtil<folly::IPAddressV6>;

} // namespace facebook::fboss::utility
