/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/Platform.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleFactory.h"
#include "fboss/agent/hw/test/HwTeFlowTestUtils.h"
#include "fboss/agent/hw/test/HwTestTeFlowUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/lib/FunctionCallTimeReporter.h"

#include <folly/Benchmark.h>
#include <folly/IPAddress.h>
#include <folly/logging/xlog.h>

DEFINE_int32(teflow_scale_entries, 9216, "Teflow scale entries");

namespace facebook::fboss {

/*
 * Collect Teflow stats with 9K entries and benchmark that.
 */

BENCHMARK(HwTeFlowStatsCollection) {
  static std::string nextHopAddr("1::1");
  static std::string ifName("fboss2000");
  static int prefixLength(61);
  uint32_t numEntries = FLAGS_teflow_scale_entries;
  folly::BenchmarkSuspender suspender;
  auto ensemble = createHwEnsemble(HwSwitchEnsemble::getAllFeatures());
  auto hwSwitch = ensemble->getHwSwitch();
  std::vector<PortID> ports = {
      ensemble->masterLogicalPortIds()[0], ensemble->masterLogicalPortIds()[1]};
  CHECK_GT(ports.size(), 0);
  auto config = utility::onePortPerInterfaceConfig(
      hwSwitch, ports, cfg::PortLoopbackMode::MAC);
  ensemble->applyInitialConfig(config);
  auto ecmpHelper =
      utility::EcmpSetupAnyNPorts6(ensemble->getProgrammedState(), RouterID(0));
  // Setup EM Config
  utility::setExactMatchCfg(ensemble.get(), prefixLength);
  // Resolve nextHops
  ensemble->applyNewState(ecmpHelper.resolveNextHops(
      ensemble->getProgrammedState(), {PortDescriptor(ports[0])}));
  ensemble->applyNewState(ecmpHelper.resolveNextHops(
      ensemble->getProgrammedState(), {PortDescriptor(ports[1])}));
  // Add Entries
  auto flowEntries = utility::makeFlowEntries(
      "100", nextHopAddr, ifName, ports[0], numEntries);
  utility::addFlowEntries(ensemble.get(), flowEntries);
  CHECK_EQ(utility::getNumTeFlowEntries(hwSwitch), numEntries);
  // Measure stats collection time for 9K entries
  SwitchStats dummy;
  suspender.dismiss();
  hwSwitch->updateStats(&dummy);
  suspender.rehire();
}

} // namespace facebook::fboss
