// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/benchmarks/HwCpuLatencyBenchmarkHelper.h"

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/CpuLatencyManager.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/lib/CommonUtils.h"

#include <folly/Benchmark.h>
#include <folly/Conv.h>
#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <folly/logging/xlog.h>
#include <chrono>
#include <iostream>
#include <unordered_map>

namespace facebook::fboss {

CpuLatencyBenchmarkSetup createCpuLatencyEnsemble() {
  FLAGS_enable_cpu_latency_monitoring = true;
  FLAGS_cpu_latency_monitoring_interval_ms = 100;

  AgentEnsembleSwitchConfigFn initialConfigFn =
      [](const AgentEnsemble& ensemble) -> cfg::SwitchConfig {
    auto ports = ensemble.masterLogicalInterfacePortIds();
    CHECK_GE(ports.size(), 1);
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(), ports, true /*interfaceHasSubnet*/);
    utility::setDefaultCpuTrafficPolicyConfig(
        config, ensemble.getL3Asics(), ensemble.isSai());
    utility::addCpuQueueConfig(config, ensemble.getL3Asics(), ensemble.isSai());
    return config;
  };

  auto ensemble =
      createAgentEnsemble(initialConfigFn, false /*disableLinkStateToggler*/);

  auto allPorts = ensemble->masterLogicalInterfacePortIds();

  // Resolve NDP neighbors so CpuLatencyManager can find destination MACs.
  auto intfMac = getMacForFirstInterfaceWithPortsForTesting(
      ensemble->getProgrammedState());
  utility::EcmpSetupAnyNPorts6 ecmpHelper(
      ensemble->getProgrammedState(),
      ensemble->getSw()->needL2EntryForNeighbor(),
      intfMac);
  ensemble->applyNewState(
      [&ecmpHelper, &allPorts](const std::shared_ptr<SwitchState>& in) {
        return ecmpHelper.resolveNextHops(
            in, static_cast<unsigned int>(allPorts.size()));
      });

  XLOG(INFO) << "CpuLatencyBenchmark: ensemble setup complete for "
             << allPorts.size() << " ports";

  return {std::move(ensemble), allPorts};
}

void runCpuLatencyBenchmark() {
  folly::BenchmarkSuspender suspender;
  auto setup = createCpuLatencyEnsemble();
  suspender.dismiss();

  auto* mgr = setup.ensemble->getSw()->getCpuLatencyManager();
  CHECK(mgr) << "CpuLatencyManager not created — flag not set?";

  std::unordered_map<PortID, CpuLatencyPortStats> finalStats;
  WITH_RETRIES_N_TIMED(600, std::chrono::milliseconds(100), {
    auto stats = mgr->getAllCpuLatencyPortStats();
    bool allReported = true;
    for (const auto& portId : setup.ports) {
      auto it = stats.find(portId);
      if (it == stats.end() || *it->second.latencyUs() <= 0.0) {
        allReported = false;
        break;
      }
    }
    if (allReported) {
      finalStats = std::move(stats);
    }
    EXPECT_EVENTUALLY_TRUE(allReported);
  });

  suspender.rehire();

  CHECK(!finalStats.empty()) << "No CPU latency measurements received";

  // Compute aggregate stats.
  double maxUs = 0.0;
  double sumUs = 0.0;
  int count = 0;
  for (const auto& portId : setup.ports) {
    auto it = finalStats.find(portId);
    if (it != finalStats.end() && *it->second.latencyUs() > 0.0) {
      double us = *it->second.latencyUs();
      sumUs += us;
      maxUs = std::max(maxUs, us);
      count++;
    }
  }
  if (count != static_cast<int>(setup.ports.size())) {
    XLOG(WARNING) << "Only " << count << "/" << setup.ports.size()
                  << " ports reported latency";
  }
  double avgUs = count > 0 ? sumUs / count : 0.0;

  XLOG(INFO) << "Switch-level CPU latency: avg=" << avgUs << "us max=" << maxUs
             << "us across " << count << " ports";

  if (FLAGS_json) {
    folly::dynamic json = folly::dynamic::object;
    json["num_ports"] = count;
    json["latency_avg_us"] = avgUs;
    json["latency_max_us"] = maxUs;
    for (const auto& portId : setup.ports) {
      auto it = finalStats.find(portId);
      if (it != finalStats.end()) {
        auto key = folly::to<std::string>(
            "port_", static_cast<int>(portId), "_latency_us");
        json[key] = *it->second.latencyUs();
      }
    }
    std::cout << toPrettyJson(json) << std::endl;
  } else {
    XLOG(INFO) << "=== CPU Latency Benchmark ===";
    XLOG(INFO) << "Ports=" << count << " avg=" << avgUs << "us max=" << maxUs
               << "us";
  }
}

} // namespace facebook::fboss
