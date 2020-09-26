/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/benchmarks/HwInitToConfigBenchmarkHelper.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleFactory.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"

using namespace facebook::fboss;

DEFINE_bool(
    setup_for_warmboot,
    false,
    "Set to true will prepare the device for warmboot");

namespace facebook::fboss::utility {

void initToConfigBenchmarkHelper(
    cfg::PortSpeed uplinkPortSpeed,
    cfg::PortSpeed downlinkPortSpeed) {
  auto ensemble = createHwEnsemble(HwSwitchEnsemble::getAllFeatures());
  folly::BenchmarkSuspender suspender;
  auto hwSwitch = ensemble->getHwSwitch();
  /*
   * Based on the uplink/downlink speed, use the ConfigFactory to create
   * agent config to mimic the production config. For instance, in TH,
   * 100Gx10G as config type will create 100G uplinks and 10G downlinks
   */
  auto config = utility::createUplinkDownlinkConfig(
      hwSwitch,
      ensemble->masterLogicalPortIds(),
      uplinkPortSpeed,
      downlinkPortSpeed,
      cfg::PortLoopbackMode::MAC);
  /*
   * This is to measure the performance when the config is applied during
   * coldboot/warmboot. This measures the time agent took to transition
   * from INITIALIZED to CONFIGURED.
   * We reuse initToConfigBenchmarkHelper for both coldboot init and
   * warmboot setup. Enable benchmarking only for coldboot/warmbot init and
   * disable when setting up for warmboot
   */
  suspender.dismiss();
  ensemble->applyInitialConfig(config);
  suspender.rehire();
  if (FLAGS_setup_for_warmboot) {
    ensemble->gracefulExit();
    // Leak HwSwitchEnsemble for warmboot, so that
    // we don't run destructors and unprogram h/w. We are
    // going to exit the process anyways.
    __attribute__((unused)) auto leakedHwEnsemble = ensemble.release();
  }
}

} // namespace facebook::fboss::utility
