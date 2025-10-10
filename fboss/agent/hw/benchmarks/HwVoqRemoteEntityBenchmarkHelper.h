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

enum class RemoteEntityType {
  REMOTE_SYS_PORT_AND_INTF,
  REMOTE_NBR_ONLY,
};

inline void remoteEntityBenchmark(RemoteEntityType type, bool add) {
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

  std::map<SwitchID, std::shared_ptr<SystemPortMap>> switchId2SystemPorts;
  std::map<SwitchID, std::shared_ptr<InterfaceMap>> switchId2IntfsWithNeighbor;
  std::map<SwitchID, std::shared_ptr<InterfaceMap>>
      switchId2IntfsWithoutNeighbor;
  std::map<SwitchID, std::shared_ptr<SystemPortMap>> emptySysPortMaps;
  std::map<SwitchID, std::shared_ptr<InterfaceMap>> emptyIntfMaps;

  const auto& config = ensemble->getSw()->getConfig();
  const auto useEncapIndex =
      ensemble->getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
          HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE);

  utility::populateRemoteIntfAndSysPorts(
      switchId2SystemPorts, switchId2IntfsWithNeighbor, config, useEncapIndex);

  SwSwitch::StateUpdateFn updateDsfStateWithNeighborFn =
      [&ensemble, &switchId2SystemPorts, &switchId2IntfsWithNeighbor](
          const std::shared_ptr<SwitchState>& in) {
        return DsfStateUpdaterUtil::getUpdatedState(
            in,
            ensemble->getSw()->getScopeResolver(),
            ensemble->getSw()->getRib(),
            switchId2SystemPorts,
            switchId2IntfsWithNeighbor);
      };

  // Only populate intf without neighbor and setup initial state if testing for
  // neighbor only.
  if (type == RemoteEntityType::REMOTE_NBR_ONLY) {
    utility::populateRemoteIntfAndSysPorts(
        switchId2SystemPorts,
        switchId2IntfsWithoutNeighbor,
        config,
        useEncapIndex,
        false /*addNeighbor*/);

    SwSwitch::StateUpdateFn updateDsfStateWithoutNeighborFn =
        [&ensemble, &switchId2SystemPorts, &switchId2IntfsWithoutNeighbor](
            const std::shared_ptr<SwitchState>& in) {
          return DsfStateUpdaterUtil::getUpdatedState(
              in,
              ensemble->getSw()->getScopeResolver(),
              ensemble->getSw()->getRib(),
              switchId2SystemPorts,
              switchId2IntfsWithoutNeighbor);
        };

    ensemble->getSw()->getRib()->updateStateInRibThread(
        [&ensemble,
         updateDsfStateWithNeighborFn,
         updateDsfStateWithoutNeighborFn,
         add]() {
          ensemble->getSw()->updateStateWithHwFailureProtection(
              folly::sformat("Update state for node: {}", 0),
              add ? updateDsfStateWithoutNeighborFn
                  : updateDsfStateWithNeighborFn);
        });
  } else if (type == RemoteEntityType::REMOTE_SYS_PORT_AND_INTF && !add) {
    // Populate Empty intf and sys port maps to remove all remote entities
    for (const auto& [switchId, _] : switchId2SystemPorts) {
      emptySysPortMaps[switchId] = std::make_shared<SystemPortMap>();
      emptyIntfMaps[switchId] = std::make_shared<InterfaceMap>();
    }

    // Setup initial state with all system ports and interfaces
    ensemble->getSw()->getRib()->updateStateInRibThread(
        [&ensemble, updateDsfStateWithNeighborFn]() {
          ensemble->getSw()->updateStateWithHwFailureProtection(
              folly::sformat("Update state for node: {}", 0),
              updateDsfStateWithNeighborFn);
        });
  }

  ScopedCallTimer timeIt;
  suspender.dismiss();
  auto intfMaps = add ? switchId2IntfsWithNeighbor
                      : (type == RemoteEntityType::REMOTE_NBR_ONLY
                             ? switchId2IntfsWithoutNeighbor
                             : emptyIntfMaps);
  auto sysPortMaps = add
      ? switchId2SystemPorts
      : (type == RemoteEntityType::REMOTE_NBR_ONLY ? switchId2SystemPorts
                                                   : emptySysPortMaps);
  CHECK(!intfMaps.empty());
  if (type == RemoteEntityType::REMOTE_SYS_PORT_AND_INTF) {
    CHECK(!sysPortMaps.empty());
    CHECK_EQ(intfMaps.size(), sysPortMaps.size());
  }

  for (const auto& [switchId, intfMap] : intfMaps) {
    CHECK(sysPortMaps.contains(switchId));
    auto sysPortMap = sysPortMaps.at(switchId);
    SwSwitch::StateUpdateFn updateForSingleRemoteNode =
        [&ensemble, switchId, type, &intfMap, &sysPortMap](
            const std::shared_ptr<SwitchState>& in) {
          std::map<SwitchID, std::shared_ptr<InterfaceMap>> switchId2Intf;
          switchId2Intf[switchId] = intfMap;
          std::map<SwitchID, std::shared_ptr<SystemPortMap>> switchId2SysPort;
          if (type == RemoteEntityType::REMOTE_SYS_PORT_AND_INTF) {
            switchId2SysPort[switchId] = sysPortMap;
          }
          return DsfStateUpdaterUtil::getUpdatedState(
              in,
              ensemble->getSw()->getScopeResolver(),
              ensemble->getSw()->getRib(),
              switchId2SysPort,
              switchId2Intf);
        };
    ensemble->getSw()->getRib()->updateStateInRibThread(
        [&ensemble, switchId, updateForSingleRemoteNode]() {
          ensemble->getSw()->updateStateWithHwFailureProtection(
              folly::sformat(
                  "Update state for node: {}", static_cast<int>(switchId)),
              updateForSingleRemoteNode);
        });
  }
  suspender.rehire();
}
} // namespace facebook::fboss
