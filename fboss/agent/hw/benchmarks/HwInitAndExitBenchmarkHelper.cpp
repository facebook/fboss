/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/benchmarks/HwInitAndExitBenchmarkHelper.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleFactory.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"
#include "fboss/agent/platforms/common/PlatformMode.h"

#include <folly/logging/xlog.h>
#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <iostream>

using namespace facebook::fboss;

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

namespace facebook::fboss::utility {

std::optional<uint16_t> getUplinksCount(
    const HwSwitch* hwSwitch,
    cfg::PortSpeed uplinkSpeed,
    cfg::PortSpeed downlinkSpeed) {
  auto platformMode = hwSwitch->getPlatform()->getMode();
  using ConfigType = std::tuple<PlatformMode, cfg::PortSpeed, cfg::PortSpeed>;
  static const std::map<ConfigType, uint16_t> numUplinksMap = {
      {{PlatformMode::WEDGE, cfg::PortSpeed::FORTYG, cfg::PortSpeed::XG}, 4},
      {{PlatformMode::WEDGE100, cfg::PortSpeed::HUNDREDG, cfg::PortSpeed::XG},
       4},
      {{PlatformMode::WEDGE100,
        cfg::PortSpeed::HUNDREDG,
        cfg::PortSpeed::TWENTYFIVEG},
       4},
      {{PlatformMode::WEDGE100,
        cfg::PortSpeed::HUNDREDG,
        cfg::PortSpeed::FIFTYG},
       4},
      {{PlatformMode::GALAXY_LC,
        cfg::PortSpeed::HUNDREDG,
        cfg::PortSpeed::FIFTYG},
       16},
      {{PlatformMode::GALAXY_FC,
        cfg::PortSpeed::HUNDREDG,
        cfg::PortSpeed::FIFTYG},
       16},
      {{PlatformMode::WEDGE400C, cfg::PortSpeed::HUNDREDG, cfg::PortSpeed::XG},
       4},
      {{PlatformMode::WEDGE400C,
        cfg::PortSpeed::HUNDREDG,
        cfg::PortSpeed::TWENTYFIVEG},
       4},
      {{PlatformMode::WEDGE400C,
        cfg::PortSpeed::HUNDREDG,
        cfg::PortSpeed::FIFTYG},
       4},
      {{PlatformMode::WEDGE400C,
        cfg::PortSpeed::HUNDREDG,
        cfg::PortSpeed::HUNDREDG},
       4},
  };

  auto iter = numUplinksMap.find(
      std::make_tuple(platformMode, uplinkSpeed, downlinkSpeed));
  if (iter == numUplinksMap.end()) {
    return std::nullopt;
  }
  return iter->second;
}

void enableFeaturesInConfig(
    cfg::SwitchConfig& config,
    const HwSwitch* hwSwitch) {
  auto hwAsic = hwSwitch->getPlatform()->getAsic();
  /*
   * Configures port queue for cpu port
   */
  utility::addCpuQueueConfig(config, hwAsic);
  /*
   * Enable Olympic QOS
   */
  if (hwAsic->isSupported(HwAsic::Feature::L3_QOS)) {
    utility::addOlympicQosMaps(config);
  }
  /*
   * Enable COPP.
   */
  config.cpuTrafficPolicy_ref()->rxReasonToQueueOrderedList_ref() =
      getCoppRxReasonToQueues(hwAsic);
  /*
   * Enable Load balancer
   */
  if (hwAsic->isSupported(HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    config.loadBalancers_ref() =
        utility::getEcmpFullTrunkHalfHashConfig(hwSwitch->getPlatform());
  }
}

void initandExitBenchmarkHelper(
    cfg::PortSpeed uplinkSpeed,
    cfg::PortSpeed downlinkSpeed) {
  auto ensemble = createHwEnsemble(HwSwitchEnsemble::getAllFeatures());
  folly::BenchmarkSuspender suspender;
  auto hwSwitch = ensemble->getHwSwitch();
  auto numUplinks = getUplinksCount(hwSwitch, uplinkSpeed, downlinkSpeed);
  if (!numUplinks) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  /*
   * Based on the uplink/downlink speed, use the ConfigFactory to create
   * agent config to mimic the production config. For instance, in TH,
   * 100Gx10G as config type will create 100G uplinks and 10G downlinks
   */
  auto config = utility::createUplinkDownlinkConfig(
      hwSwitch,
      ensemble->masterLogicalPortIds(),
      numUplinks.value(),
      uplinkSpeed,
      downlinkSpeed,
      cfg::PortLoopbackMode::MAC);
  enableFeaturesInConfig(config, hwSwitch);

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
}

} // namespace facebook::fboss::utility
