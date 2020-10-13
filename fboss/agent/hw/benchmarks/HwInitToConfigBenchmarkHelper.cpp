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
#include "fboss/agent/platforms/common/PlatformMode.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"

using namespace facebook::fboss;

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

void initToConfigBenchmarkHelper(
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
