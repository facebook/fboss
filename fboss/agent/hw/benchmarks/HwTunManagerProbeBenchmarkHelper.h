/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

/**
 * @file HwTunManagerProbeBenchmarkHelper.h
 * @brief Performance benchmark helper for TunManager kernel interface probing
 * and cleanup operations
 * @author Ashutosh Grewal (agrewal@meta.com)
 */

#pragma once

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/SwAgentInitializer.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/Benchmark.h>

namespace facebook::fboss {

// Forward declaration for friend access
void runTunManagerProbeBenchmark();

} // namespace facebook::fboss

/**
 * @def HW_TUN_MANAGER_BENCHMARK_FRIEND_TESTS
 * @brief Macro defining friend declarations for TunManager benchmarks
 *
 * This macro contains friend class declarations needed to access private
 * members of TunManager for benchmarking.
 */
#define HW_TUN_MANAGER_BENCHMARK_FRIEND_TESTS \
  friend void facebook::fboss::runTunManagerProbeBenchmark();

#include "fboss/agent/TunManager.h"

namespace facebook::fboss {

/*
 * Benchmark probing and cleanup of TUN interface data.
 * This benchmarks the time it takes to:
 * 1. Probe existing kernel TUN interface state
 * 2. Perform initial cleanup of probed data
 */
inline void runTunManagerProbeBenchmark() {
  folly::BenchmarkSuspender suspender;
  std::unique_ptr<AgentEnsemble> ensemble{};

  // Set flags to enable TUN interface support
  // These must be set before creating the AgentEnsemble
  FLAGS_tun_intf = true;
  FLAGS_cleanup_probed_kernel_data = true;

  AgentEnsembleSwitchConfigFn initialConfigFn =
      [](const AgentEnsemble& ensemble) {
        auto allPorts = ensemble.masterLogicalPortIds();
        CHECK_GT(allPorts.size(), 0);

        XLOG(INFO) << "Using " << allPorts.size()
                   << " ports for TUN Manager benchmark";

        return utility::onePortPerInterfaceConfig(ensemble.getSw(), allPorts);
      };

  ensemble =
      createAgentEnsemble(initialConfigFn, false /*disableLinkStateToggler*/);

  // Apply the config and wait for state updates
  auto config = initialConfigFn(*ensemble);
  ensemble->applyNewConfig(config);
  waitForStateUpdates(ensemble->getSw());

  // Initial sync for TunManager is async and happens on the background event
  // base Wait for pending operations there to complete.
  waitForBackgroundThread(ensemble->getSw());
  waitForStateUpdates(ensemble->getSw());

  auto tunMgr = ensemble->getSw()->getTunManager();

  // Ensure we have a valid netlink socket for operations
  // There could be a race condition where the interface is up, but the
  // socket is not created. So, checking for the socket existence.
  if (!tunMgr->isValidNlSocket()) {
    XLOG(ERR) << "No valid netlink socket available for benchmark";
    return;
  }

  // Set probeDone_ to false before calling probe()
  tunMgr->probeDone_ = false;

  // Start measuring only the critical probe and cleanup operations
  suspender.dismiss();

  const auto startTs = std::chrono::steady_clock::now();

  // Probe kernel for existing TUN interface state
  tunMgr->probe();

  // Force cleanup by calling deleteAllProbedData directly. This ensures cleanup
  // happens regardless of interface mapping comparison
  tunMgr->deleteAllProbedData();

  const auto endTs = std::chrono::steady_clock::now();
  auto elapsedMs =
      std::chrono::duration_cast<std::chrono::milliseconds>(endTs - startTs);
  XLOG(INFO) << "TUN Manager probe and cleanup operations took "
             << elapsedMs.count() << "ms to complete";

  // Stop measuring
  suspender.rehire();
}

} // namespace facebook::fboss
