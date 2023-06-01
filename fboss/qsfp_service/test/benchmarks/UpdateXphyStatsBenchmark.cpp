// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>
#include <unordered_set>
#include "fboss/lib/CommonUtils.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"
#include "fboss/qsfp_service/test/benchmarks/HwBenchmarkUtils.h"

namespace facebook::fboss {

std::set<PortID> getEnabledPorts(const WedgeManager* wedgeManager) {
  std::set<PortID> enabledPorts;
  auto qsfpTestConfig = wedgeManager->getQsfpConfig()->thrift.qsfpTestConfig();
  CHECK(qsfpTestConfig.has_value());
  for (const auto& cabledPairs : *qsfpTestConfig->cabledPortPairs()) {
    auto& aPortName = *cabledPairs.aPortName();
    auto& zPortName = *cabledPairs.zPortName();
    auto aPortId = wedgeManager->getPortIDByPortName(aPortName);
    auto zPortId = wedgeManager->getPortIDByPortName(zPortName);
    CHECK(aPortId.has_value());
    CHECK(zPortId.has_value());
    enabledPorts.insert(*aPortId);
    enabledPorts.insert(*zPortId);
  }
  return enabledPorts;
}

size_t updateXphyStats() {
  // Don't account for the time spent in setup
  folly::BenchmarkSuspender suspender;

  // IPHY is not programmed in this test, hence override its programming
  gflags::SetCommandLineOptionWithMode(
      "override_program_iphy_ports_for_test", "1", gflags::SET_FLAGS_DEFAULT);
  auto wedgeMgr = setupForColdboot();
  wedgeMgr->init();

  // Filter out enabled ports on the config that have XPHY in the datapath
  auto xphyPorts = wedgeMgr->getPhyManager()->getXphyPorts();
  auto enabledPorts = getEnabledPorts(wedgeMgr.get());
  std::vector<PortID> enabledXphyPorts;
  for (auto port : xphyPorts) {
    if (enabledPorts.find(port) != enabledPorts.end()) {
      enabledXphyPorts.push_back(port);
    }
  }

  // Refresh state machines till all enabled XPHY ports are programmed
  auto refreshStateMachinesTillXphyProgrammed = [&wedgeMgr,
                                                 &enabledXphyPorts]() {
    wedgeMgr->refreshStateMachines();
    for (auto id : enabledXphyPorts) {
      if (!wedgeMgr->getPhyManager()->getProgrammedSpeed(id).has_value()) {
        return false;
      }
    }
    return true;
  };

  // Retry until all enabled XPHY ports get programmed
  checkWithRetry(
      refreshStateMachinesTillXphyProgrammed,
      6 /* retries */,
      std::chrono::milliseconds(5000) /* msBetweenRetry */,
      "Never got all xphys programmed");

  // Wait till stats collection is done at least once on all enabled xphy ports
  auto waitForStatsCollectionDone = [&wedgeMgr, &enabledXphyPorts]() {
    for (auto port : enabledXphyPorts) {
      if (!wedgeMgr->getPhyManager()->isXphyStatsCollectionDone(port)) {
        return false;
      }
    }
    return true;
  };
  suspender.dismiss();
  // Start benchmarking
  wedgeMgr->updateAllXphyPortsStats();
  checkWithRetry(
      waitForStatsCollectionDone,
      90 * 1000 /* retry for 90 seconds */,
      std::chrono::milliseconds(1) /* msBetweenRetry */,
      "Never got xphy stats collection done");
  // End benchmarking
  suspender.rehire();

  return enabledXphyPorts.size();
}

// Runs updateAllXphyPortsStats for the entire system and benchmarks the time
// taken to update one port
BENCHMARK_MULTI(UpdateXphyStats) {
  return updateXphyStats();
}

} // namespace facebook::fboss
