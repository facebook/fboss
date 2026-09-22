// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/MultiPortTrafficTestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"

/*
 * S416694: heavy congestion on a few ports consumed the entire device buffer,
 * so traffic to ports that were not themselves congested was dropped due to
 * buffer unavailability. The fix caps overall device buffer utilization and
 * reserves a minimum per queue.
 *
 * Scenario under test: congest a few output queues hard enough to put the
 * device under sustained buffer pressure, and check that buffer usage does not
 * cross a specified threshold at the heaviest load, which guarantees that
 * enough buffer is left for traffic on queues that are not congested.
 *
 * Buffer model, thresholds and the values below:
 * https://docs.google.com/document/d/1e6STyTlR5M_M9GVYJh08H5We612hPObEU8ijJF6QBEI/edit?usp=sharing
 */

namespace facebook::fboss {

namespace {

constexpr int kNumCongestedPorts = 2;

constexpr int kNumLoopPorts = 1;

} // namespace

class AgentDeviceBufferGuaranteeTest : public AgentHwTest {
 protected:
  // Ports whose queues the test fills, and the ports that fill them.
  struct CongestionTargets {
    std::vector<PortID> congested;
    std::vector<folly::IPAddressV6> congestedIps;
    std::vector<PortID> injection;
  };

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = AgentHwTest::initialConfig(ensemble);
    utility::addOlympicQueueConfig(&config, ensemble.getL3Asics());
    utility::addOlympicQosMaps(config, ensemble.getL3Asics());
    // Only ECN-capable traffic is allowed to grow a queue far enough to drive
    // device utilization to the cap; non-ECN traffic is clamped well below it.
    utility::addQueueEcnConfig(
        config,
        ensemble.getL3Asics(),
        utility::kOlympicSilverQueueId,
        utility::kQueueConfigAqmsEcnThresholdMinMax,
        utility::kQueueConfigAqmsEcnThresholdMinMax,
        100 /* probability */,
        ensemble.getSw()->getSwitchInfoTable().l3SwitchType() ==
            cfg::SwitchType::VOQ);
    return config;
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::L3_QOS,
        ProductionFeature::OLYMPIC_QOS,
        ProductionFeature::ECN};
  }

  // Opt out of the 8 port default: eight senders cannot drive utilization far
  // enough to reach the cap.
  std::optional<size_t> maxRequiredInterfacePorts() const override {
    return std::nullopt;
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    // Keep control traffic from interfering with the test.
    FLAGS_disable_neighbor_solicitation = true;
  }

  CongestionTargets congestionTargets() const {
    const auto ports = getAgentEnsemble()->masterLogicalInterfacePortIds();
    const auto ips =
        utility::getOneRemoteHostIpPerInterfacePort(getAgentEnsemble());
    CHECK_GT(ports.size(), kNumCongestedPorts + kNumLoopPorts)
        << "need more interface ports to congest some and keep one draining";
    CongestionTargets targets;
    // Route and adjacency are programmed for the loop port first, then for
    // the ports where congestion is created. Rest of the ports get neither.
    for (int i = 0; i < kNumCongestedPorts; i++) {
      targets.congested.push_back(ports[i]);
      targets.congestedIps.push_back(ips[kNumLoopPorts + i]);
    }
    targets.injection = {
        ports.begin() + kNumCongestedPorts + kNumLoopPorts, ports.end()};
    return targets;
  }
};

} // namespace facebook::fboss
