/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/DsfStateUpdaterUtil.h"
#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/utils/DsfConfigUtils.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"
#include "fboss/lib/FunctionCallTimeReporter.h"

#include <folly/Benchmark.h>

namespace facebook::fboss {

BENCHMARK(HwVoqSysPortProgramming) {
  folly::BenchmarkSuspender suspender;

  AgentEnsembleSwitchConfigFn voqInitialConfig =
      [](const AgentEnsemble& ensemble) {
        FLAGS_hide_fabric_ports = false;
        FLAGS_dsf_subscribe = false;
        auto config = utility::onePortPerInterfaceConfig(
            ensemble.getSw(),
            ensemble.masterLogicalPortIds(),
            true, /*interfaceHasSubnet*/
            true, /*setInterfaceMac*/
            utility::kBaseVlanId,
            true /*enable fabric ports*/);
        config.dsfNodes() = *utility::addRemoteIntfNodeCfg(*config.dsfNodes());
        return config;
      };
  auto ensemble =
      createAgentEnsemble(voqInitialConfig, false /*disableLinkStateToggler*/);
  ScopedCallTimer timeIt;

  auto updateDsfStateFn = [&ensemble](const std::shared_ptr<SwitchState>& in) {
    std::map<SwitchID, std::shared_ptr<SystemPortMap>> switchId2SystemPorts;
    std::map<SwitchID, std::shared_ptr<InterfaceMap>> switchId2Rifs;
    utility::populateRemoteIntfAndSysPorts(
        switchId2SystemPorts,
        switchId2Rifs,
        ensemble->getSw()->getConfig(),
        ensemble->getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
            HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE));
    return DsfStateUpdaterUtil::getUpdatedState(
        in,
        ensemble->getSw()->getScopeResolver(),
        ensemble->getSw()->getRib(),
        switchId2SystemPorts,
        switchId2Rifs);
  };

  suspender.dismiss();
  ensemble->getSw()->getRib()->updateStateInRibThread(
      [&ensemble, updateDsfStateFn]() {
        ensemble->getSw()->updateStateWithHwFailureProtection(
            folly::sformat("Update state for node: {}", 0), updateDsfStateFn);
      });
  suspender.rehire();
}
} // namespace facebook::fboss
