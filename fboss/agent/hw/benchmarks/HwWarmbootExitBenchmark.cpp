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
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleFactory.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/RouteScaleGenerators.h"

#include <folly/IPAddressV6.h>
#include <folly/dynamic.h>
#include <folly/init/Init.h>
#include <folly/json.h>

#include <chrono>
#include <iostream>

DEFINE_bool(json, true, "Output in json form");
DEFINE_bool(
    setup_for_warmboot,
    false,
    "Set to true will prepare the device for warmboot");

namespace {
class StopWatch {
 public:
  StopWatch() : startTime_(std::chrono::steady_clock::now()) {}
  ~StopWatch() {
    std::chrono::duration<double, std::milli> durationMillseconds =
        std::chrono::steady_clock::now() - startTime_;
    if (FLAGS_json) {
      folly::dynamic warmBootTime = folly::dynamic::object;
      warmBootTime["warm_boot_msecs"] = durationMillseconds.count();
      std::cout << warmBootTime << std::endl;
    } else {
      XLOG(INFO) << " warm boot msecs: " << durationMillseconds.count();
    }
  }

 private:
  std::chrono::time_point<std::chrono::steady_clock> startTime_;
};
} // namespace
namespace facebook::fboss {

void runBenchmark() {
  auto ensemble = createHwEnsemble(HwSwitchEnsemble::getAllFeatures());
  auto hwSwitch = ensemble->getHwSwitch();
  auto config =
      utility::onePortPerVlanConfig(hwSwitch, ensemble->masterLogicalPortIds());
  ensemble->applyInitialConfig(config);

  std::shared_ptr<SwitchState> toApply;
  if (ensemble->getPlatform()->getMode() == PlatformMode::WEDGE) {
    toApply = utility::RouteDistributionGenerator(
                  ensemble->getProgrammedState(),
                  // Distribution taken from a random wedge in fleet
                  {
                      // v6 distribution - mask to num unique prefixes
                      {37, 7},
                      {44, 11},
                      {47, 6},
                      {48, 3},
                      {52, 61},
                      {54, 340},
                      {64, 1542},
                      {80, 1},
                      {127, 4},
                  },
                  {// v4 distribution - mask to num unique prefixes
                   {15, 4},
                   {16, 1},
                   {17, 1},
                   {21, 10},
                   {24, 31},
                   {26, 6},
                   {27, 26},
                   {31, 40}},
                  4000,
                  4)
                  .getSwitchStates()
                  .back();
  } else {
    toApply = utility::FSWRouteScaleGenerator(ensemble->getProgrammedState())
                  .getSwitchStates()
                  .back();
  }
  ensemble->applyNewState(toApply);
  // Static such that the object destructor runs as late as possible. In
  // Static such that the object destructor runs as late as possible. In
  // particular in this case, destructor (and thus the duration calculation)
  // will run at the time of program exit when static variable destructors run
  static StopWatch timer;
  ensemble->gracefulExit();
  // Leak HwSwitchEnsemble for warmboot, so that
  // we don't run destructors and unprogram h/w. We are
  // going to exit the process anyways.
  __attribute__((unused)) auto leakedHwEnsemble = ensemble.release();
}
} // namespace facebook::fboss

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv, true);
  facebook::fboss::runBenchmark();
  return 0;
}
