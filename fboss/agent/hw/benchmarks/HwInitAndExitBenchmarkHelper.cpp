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
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleRouteUpdateWrapper.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestProdConfigUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"
#include "fboss/agent/platforms/common/PlatformMode.h"
#include "fboss/agent/test/RouteDistributionGenerator.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/lib/FunctionCallTimeReporter.h"

#include <folly/logging/xlog.h>
#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <iostream>

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
      {{PlatformMode::WEDGE400,
        cfg::PortSpeed::HUNDREDG,
        cfg::PortSpeed::TWENTYFIVEG},
       4},
      {{PlatformMode::WEDGE400,
        cfg::PortSpeed::HUNDREDG,
        cfg::PortSpeed::FIFTYG},
       4},
      {{PlatformMode::WEDGE400,
        cfg::PortSpeed::HUNDREDG,
        cfg::PortSpeed::HUNDREDG},
       4},
      {{PlatformMode::YAMP, cfg::PortSpeed::HUNDREDG, cfg::PortSpeed::HUNDREDG},
       4},
      {{PlatformMode::MINIPACK,
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

utility::RouteDistributionGenerator::ThriftRouteChunks getRoutes(
    const HwSwitchEnsemble* ensemble) {
  /*
   * |  Platform   |  Role  |
   * |  TRIDENT2   |   RSW  |
   * |  TOMAHAWK   |   FSW  |
   * |  TOMAHAWK3  |    UU  |
   * |  TAJO       |   RSW  |
   *
   * The benchmarks are categorized by chip and not by the route scale.
   * Pick the highest scale for that chip. For instance, any TH asic will
   * be categorized as th_atom_init_and_exit_100Gx100G which is applicable
   * for wedge100, wedge100S and Galaxy. Hence, use FSW route scale for TH.
   */
  auto asicType =
      ensemble->getHwSwitch()->getPlatform()->getAsic()->getAsicType();

  if (asicType == HwAsic::AsicType::ASIC_TYPE_TRIDENT2) {
    return utility::RSWRouteScaleGenerator(ensemble->getProgrammedState())
        .getThriftRoutes();
  } else if (
      asicType == HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3 ||
      asicType == HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4 ||
      asicType == HwAsic::AsicType::ASIC_TYPE_TAJO) {
    return utility::HgridUuRouteScaleGenerator(ensemble->getProgrammedState())
        .getThriftRoutes();
  } else if (asicType == HwAsic::AsicType::ASIC_TYPE_TOMAHAWK) {
    return utility::FSWRouteScaleGenerator(ensemble->getProgrammedState())
        .getThriftRoutes();
  } else {
    CHECK(false) << "Invalid asic type for route scale";
  }
}

void initandExitBenchmarkHelper(
    cfg::PortSpeed uplinkSpeed,
    cfg::PortSpeed downlinkSpeed) {
  folly::BenchmarkSuspender suspender;
  std::unique_ptr<HwSwitchEnsemble> ensemble;
  suspender.dismiss();
  {
    /*
     * Measure the hw switch init time
     */
    ScopedCallTimer timeIt;
    ensemble = createHwEnsemble(HwSwitchEnsemble::getAllFeatures());
  }
  suspender.rehire();
  auto hwSwitch = ensemble->getHwSwitch();
  auto numUplinks = getUplinksCount(hwSwitch, uplinkSpeed, downlinkSpeed);
  if (!numUplinks) {
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
  utility::addProdFeaturesToConfig(config, hwSwitch);

  /*
   * This is to measure the performance when the config is applied during
   * coldboot/warmboot. This measures the time agent took to transition
   * from INITIALIZED to CONFIGURED.
   * We reuse initToConfigBenchmarkHelper for both coldboot init and
   * warmboot setup. Enable benchmarking only for coldboot/warmbot init and
   * disable when setting up for warmboot
   */
  suspender.dismiss();
  {
    ScopedCallTimer timeIt;
    /*
     * Do not apply the config through HwSwitchEnsemble::applyInitialConfig
     * since it disables and enables the port and waits for the port to be UP
     * before returning. We would like to measure the performance only for hw
     * switch init and also the state transition from INIT TO CONFIGURED.
     */
    ensemble->applyNewConfig(config);
    ensemble->switchRunStateChanged(SwitchRunState::CONFIGURED);
  }
  suspender.rehire();
  auto routeChunks = getRoutes(ensemble.get());
  auto updater = ensemble->getRouteUpdater();
  updater.programRoutes(RouterID(0), ClientID::BGPD, routeChunks);
  if (FLAGS_setup_for_warmboot) {
    ScopedCallTimer timeIt;
    // Static such that the object destructor runs as late as possible. In
    // particular in this case, destructor (and thus the duration calculation)
    // will run at the time of program exit when static variable destructors
    // run
    static StopWatch timer("warm_boot_msecs", FLAGS_json);
    ensemble->gracefulExit();
    // Leak HwSwitchEnsemble for warmboot, so that
    // we don't run destructors and unprogram h/w. We are
    // going to exit the process anyways.
    __attribute__((unused)) auto leakedHwEnsemble = ensemble.release();
  }
}
} // namespace facebook::fboss::utility
