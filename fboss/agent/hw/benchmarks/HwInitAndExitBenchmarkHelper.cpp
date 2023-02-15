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
#include <fboss/agent/SwSwitch.h>
#include <fboss/agent/test/AgentEnsemble.h>
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
#include "fboss/agent/test/RouteDistributionGenerator.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/lib/FunctionCallTimeReporter.h"
#include "fboss/lib/platforms/PlatformMode.h"

#include <folly/logging/xlog.h>
#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <iostream>

#include "fboss/agent/benchmarks/AgentBenchmarks.h"

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
    const AgentEnsemble* ensemble) {
  /*
   * |  Platform   |  Role  |
   * |  TRIDENT2   |   RSW  |
   * |  TOMAHAWK   |   FSW  |
   * |  TOMAHAWK3  |    UU  |
   * |  EBRO       |   RSW  |
   *
   * The benchmarks are categorized by chip and not by the route scale.
   * Pick the highest scale for that chip. For instance, any TH asic will
   * be categorized as th_atom_init_and_exit_100Gx100G which is applicable
   * for wedge100, wedge100S and Galaxy. Hence, use FSW route scale for TH.
   */
  auto* swSwitch = ensemble->getSw();
  auto asicType = ensemble->getHw()->getPlatform()->getAsic()->getAsicType();

  if (asicType == cfg::AsicType::ASIC_TYPE_TRIDENT2) {
    return utility::RSWRouteScaleGenerator(swSwitch->getState())
        .getThriftRoutes();
  } else if (
      asicType == cfg::AsicType::ASIC_TYPE_TOMAHAWK3 ||
      asicType == cfg::AsicType::ASIC_TYPE_TOMAHAWK4 ||
      asicType == cfg::AsicType::ASIC_TYPE_EBRO ||
      asicType == cfg::AsicType::ASIC_TYPE_GARONNE ||
      asicType == cfg::AsicType::ASIC_TYPE_INDUS ||
      asicType == cfg::AsicType::ASIC_TYPE_BEAS) {
    return utility::HgridUuRouteScaleGenerator(swSwitch->getState())
        .getThriftRoutes();
  } else if (asicType == cfg::AsicType::ASIC_TYPE_TOMAHAWK) {
    return utility::FSWRouteScaleGenerator(swSwitch->getState())
        .getThriftRoutes();
  } else {
    CHECK(false) << "Invalid asic type for route scale";
  }
}

void initandExitBenchmarkHelper(
    cfg::PortSpeed uplinkSpeed,
    cfg::PortSpeed downlinkSpeed) {
  folly::BenchmarkSuspender suspender;
  std::unique_ptr<AgentEnsemble> ensemble{};

  AgentEnsembleSwitchConfigFn initialConfig =
      [uplinkSpeed, downlinkSpeed](
          HwSwitch* hwSwitch, const std::vector<PortID>& ports) {
        auto numUplinks = getUplinksCount(hwSwitch, uplinkSpeed, downlinkSpeed);
        if (!numUplinks) {
          return utility::oneL3IntfNPortConfig(hwSwitch, ports);
        }
        /*
         * Based on the uplink/downlink speed, use the ConfigFactory to create
         * agent config to mimic the production config. For instance, in TH,
         * 100Gx10G as config type will create 100G uplinks and 10G downlinks
         */

        auto config = utility::createUplinkDownlinkConfig(
            hwSwitch,
            ports,
            numUplinks.value(),
            uplinkSpeed,
            downlinkSpeed,
            hwSwitch->getPlatform()->getAsic()->desiredLoopbackMode());
        utility::addProdFeaturesToConfig(config, hwSwitch);
        return config;
      };

  suspender.dismiss();
  {
    /*
     * Measure the hw switch init time
     */
    /*
     * This is to measure the performance when the config is applied during
     * coldboot/warmboot. This measures the time agent took to transition
     * from INITIALIZED to CONFIGURED.
     * We reuse initToConfigBenchmarkHelper for both coldboot init and
     * warmboot setup. Enable benchmarking only for coldboot/warmbot init and
     * disable when setting up for warmboot
     */
    ScopedCallTimer timeIt;
    ensemble = createAgentEnsemble(initialConfig);
    ensemble->startAgent();
  }
  suspender.rehire();
  auto routeChunks = getRoutes(ensemble.get());
  auto updater = ensemble->getSw()->getRouteUpdater();
  ensemble->programRoutes(RouterID(0), ClientID::BGPD, routeChunks);
  if (FLAGS_setup_for_warmboot) {
    ScopedCallTimer timeIt;
    // Static such that the object destructor runs as late as possible. In
    // particular in this case, destructor (and thus the duration calculation)
    // will run at the time of program exit when static variable destructors
    // run
    static StopWatch timer("warm_boot_msecs", FLAGS_json);
    ensemble.reset();
  }
}
} // namespace facebook::fboss::utility
