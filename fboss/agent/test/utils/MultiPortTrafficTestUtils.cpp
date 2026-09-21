// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/MultiPortTrafficTestUtils.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
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

void setupEcmpDataplaneLoopOnPorts(
    facebook::fboss::AgentEnsemble* ensemble,
    const std::vector<PortID>& ports) {
  auto intfMac = getMacForFirstInterfaceWithPortsForTesting(
      ensemble->getProgrammedState());
  utility::EcmpSetupTargetedPorts6 ecmpHelper(
      ensemble->getProgrammedState(),
      ensemble->getSw()->needL2EntryForNeighbor(),
      intfMac);
  std::vector<PortDescriptor> portDescriptors;
  std::vector<boost::container::flat_set<PortDescriptor>> portDescSets;
  for (auto& portId : ports) {
    portDescriptors.emplace_back(portId);
    portDescSets.push_back(
        boost::container::flat_set<PortDescriptor>{PortDescriptor(portId)});
  }
  ensemble->applyNewState(
      [&portDescriptors, &ecmpHelper](const std::shared_ptr<SwitchState>& in) {
        return ecmpHelper.resolveNextHops(
            in,
            boost::container::flat_set<PortDescriptor>(
                std::make_move_iterator(portDescriptors.begin()),
                std::make_move_iterator(portDescriptors.end())));
      });

  std::vector<RoutePrefixV6> routePrefixes;
  routePrefixes.reserve(ports.size());
  for (size_t i = 0; i < ports.size(); i++) {
    routePrefixes.emplace_back(
        folly::IPAddressV6(folly::to<std::string>("2401::", i + 1)), 128);
  }
  auto routeUpdater = ensemble->getSw()->getRouteUpdater();
  ecmpHelper.programRoutes(
      &routeUpdater, portDescSets, routePrefixes, {}, std::nullopt, true);
  for (auto& nhop : ecmpHelper.getNextHops()) {
    utility::disablePortTTLDecrementIfSupported(
        ensemble, ecmpHelper.getRouterId(), nhop);
  }
}

void setupEcmpDataplaneLoopOnAllPorts(
    facebook::fboss::AgentEnsemble* ensemble) {
  auto ports = FLAGS_hyper_port ? ensemble->masterLogicalHyperPortIds()
                                : ensemble->masterLogicalInterfacePortIds();
  setupEcmpDataplaneLoopOnPorts(ensemble, ports);
}

void createTrafficOnMultiplePorts(
    facebook::fboss::AgentEnsemble* ensemble,
    int numberOfPorts,
    const std::function<void(
        facebook::fboss::AgentEnsemble* ensemble,
        const folly::IPAddressV6&)>& sendPacketFn,
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

void waitForLineRateOnPorts(
    facebook::fboss::AgentEnsemble* ensemble,
    const std::vector<PortID>& ports,
    double desiredPctLineRate,
    int maxRetries) {
  constexpr int kWaitSeconds = 1;
  double threshold = desiredPctLineRate / 100.0;

  std::set<PortID> pendingPorts(ports.begin(), ports.end());

  // Collect baseline stats before entering the retry loop.
  // Use maxRetries + 1 because the first iteration's stats collection
  // is back-to-back with this baseline (no sleep gap before the first
  // retry), so the computed rate won't be meaningful on that first check.
  std::map<PortID, HwPortStats> prevStats;
  for (const auto& portId : pendingPorts) {
    prevStats[portId] = ensemble->getLatestPortStats(portId);
  }

  WITH_RETRIES_N_TIMED(
      maxRetries + 1, std::chrono::milliseconds(1000 * kWaitSeconds), {
        std::vector<PortID> convergedPorts;
        for (const auto& portId : pendingPorts) {
          auto curStats = ensemble->getLatestPortStats(portId);
          auto rateBps = ensemble->getTrafficRate(
              prevStats[portId], curStats, kWaitSeconds);
          auto portSpeedBps =
              static_cast<uint64_t>(ensemble->getProgrammedState()
                                        ->getPorts()
                                        ->getNodeIf(portId)
                                        ->getSpeed()) *
              1000 * 1000;

          // Update baseline for next iteration
          prevStats[portId] = curStats;

          if (rateBps >= portSpeedBps * threshold) {
            convergedPorts.push_back(portId);
          }
        }

        for (const auto& portId : convergedPorts) {
          pendingPorts.erase(portId);
          prevStats.erase(portId);
        }
        XLOG(DBG2) << "Line rate check: " << convergedPorts.size()
                   << " converged, " << pendingPorts.size() << " pending";

        EXPECT_EVENTUALLY_TRUE(pendingPorts.empty());
      });

  if (!pendingPorts.empty()) {
    for (const auto& portId : pendingPorts) {
      XLOG(ERR) << "Port "
                << ensemble->getProgrammedState()
                       ->getPorts()
                       ->getNodeIf(portId)
                       ->getName()
                << " did not reach " << desiredPctLineRate
                << "% line rate after " << maxRetries << " retries";
    }
  }
}

} // namespace facebook::fboss::utility
