// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/MultiPortTrafficTestUtils.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/QosTestUtils.h"

namespace facebook::fboss::utility {
std::vector<folly::IPAddressV6> getOneRemoteHostIpPerInterfacePort(
    facebook::fboss::AgentEnsemble* ensemble) {
  std::vector<folly::IPAddressV6> ips;
  for (int idx = 1; idx <= ensemble->masterLogicalInterfacePortIds().size();
       idx++) {
    ips.emplace_back(folly::to<std::string>("2401::", idx));
  }
  return ips;
}

std::vector<folly::IPAddressV6> getOneRemoteHostIpPerHyperPort(
    facebook::fboss::AgentEnsemble* ensemble) {
  std::vector<folly::IPAddressV6> ips;
  for (int idx = 1; idx <= ensemble->masterLogicalHyperPortIds().size();
       idx++) {
    ips.emplace_back(folly::to<std::string>("2401::", idx));
  }
  return ips;
}

void setupEcmpDataplaneLoopOnAllPorts(
    facebook::fboss::AgentEnsemble* ensemble) {
  auto intfMac =
      utility::getMacForFirstInterfaceWithPorts(ensemble->getProgrammedState());
  utility::EcmpSetupTargetedPorts6 ecmpHelper(
      ensemble->getProgrammedState(),
      ensemble->getSw()->needL2EntryForNeighbor(),
      intfMac);
  std::vector<PortDescriptor> portDescriptors;
  std::vector<flat_set<PortDescriptor>> portDescSets;
  for (auto& portId :
       (FLAGS_hyper_port ? ensemble->masterLogicalHyperPortIds()
                         : ensemble->masterLogicalInterfacePortIds())) {
    portDescriptors.emplace_back(portId);
    portDescSets.push_back(flat_set<PortDescriptor>{PortDescriptor(portId)});
  }
  ensemble->applyNewState(
      [&portDescriptors, &ecmpHelper](const std::shared_ptr<SwitchState>& in) {
        return ecmpHelper.resolveNextHops(
            in,
            flat_set<PortDescriptor>(
                std::make_move_iterator(portDescriptors.begin()),
                std::make_move_iterator(portDescriptors.end())));
      });

  std::vector<RoutePrefixV6> routePrefixes;
  for (auto prefix :
       (FLAGS_hyper_port ? getOneRemoteHostIpPerHyperPort(ensemble)
                         : getOneRemoteHostIpPerInterfacePort(ensemble))) {
    routePrefixes.emplace_back(prefix, 128);
  }
  auto routeUpdater = ensemble->getSw()->getRouteUpdater();
  ecmpHelper.programRoutes(&routeUpdater, portDescSets, routePrefixes);
  for (auto& nhop : ecmpHelper.getNextHops()) {
    utility::ttlDecrementHandlingForLoopbackTraffic(
        ensemble, ecmpHelper.getRouterId(), nhop);
  }
}

void createTrafficOnMultiplePorts(
    facebook::fboss::AgentEnsemble* ensemble,
    int numberOfPorts,
    std::function<void(
        facebook::fboss::AgentEnsemble* ensemble,
        const folly::IPAddressV6&)> sendPacketFn,
    double desiredPctLineRate) {
  auto minPktsForLineRate = ensemble->getMinPktsForLineRate(
      (FLAGS_hyper_port ? ensemble->masterLogicalHyperPortIds()[0]
                        : ensemble->masterLogicalInterfacePortIds()[0]));
  auto hostIps = FLAGS_hyper_port
      ? getOneRemoteHostIpPerHyperPort(ensemble)
      : getOneRemoteHostIpPerInterfacePort(ensemble);
  for (int idx = 0; idx < numberOfPorts; idx++) {
    for (int count = 0; count < minPktsForLineRate; count++) {
      sendPacketFn(ensemble, hostIps[idx]);
    }
  }
  // Now, make sure that we have line rate traffic on these ports!
  for (int idx = 0; idx < numberOfPorts; idx++) {
    auto portId = FLAGS_hyper_port
        ? ensemble->masterLogicalHyperPortIds()[idx]
        : ensemble->masterLogicalInterfacePortIds()[idx];
    uint64_t desiredRate = static_cast<uint64_t>(ensemble->getProgrammedState()
                                                     ->getPorts()
                                                     ->getNodeIf(portId)
                                                     ->getSpeed()) *
        1000 * 1000 * desiredPctLineRate / 100;
    ensemble->waitForSpecificRateOnPort(portId, desiredRate);
  }
}

} // namespace facebook::fboss::utility
