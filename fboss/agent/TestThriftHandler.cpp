/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/TestThriftHandler.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/NdpCache.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/Utils.h"

using facebook::network::toIPAddress;

namespace {

// Systemd service names for test binaries.
const std::string kSwAgentTestService = "fboss_sw_agent_test";
// hw_agent: prefix; append switchIndex for the full systemd unit name
// (e.g. "fboss_hw_agent_for_testing_0").
const std::string kHwAgentTestServicePrefix = "fboss_hw_agent_for_testing_";

const std::set<std::string> kServicesSupportingRestart() {
  static const std::set<std::string> servicesSupportingRestart = {
      kSwAgentTestService,
      "fsdb_service_for_testing",
      "qsfp_service_for_testing",
      "bgp",
  };

  return servicesSupportingRestart;
}

// The systemd service name differs from the actual binary/process name.
// pkill matches against the process name, so we need to map service names
// to the actual binary names.
const std::map<std::string, std::string>& kServiceToProcessName() {
  static const std::map<std::string, std::string> serviceToProcessName = {
      {kSwAgentTestService, kSwAgentTestService},
      {"fsdb_service_for_testing", "fsdb"},
      {"qsfp_service_for_testing", "qsfp_service"},
      {"bgp", "bgp"},
  };

  return serviceToProcessName;
}

std::set<int16_t> getSwitchIndicesImpl(facebook::fboss::SwSwitch* sw) {
  std::set<int16_t> switchIndices;
  for (const auto& [switchId, switchInfo] :
       sw->getSwitchInfoTable().getSwitchIdToSwitchInfo()) {
    switchIndices.insert(*switchInfo.switchIndex());
  }
  return switchIndices;
}

} // namespace

namespace facebook::fboss {

TestThriftHandler::TestThriftHandler(SwSwitch* sw) : ThriftHandler(sw) {}

void TestThriftHandler::gracefullyRestartService(
    std::unique_ptr<std::string> serviceName) {
  XLOG(INFO) << __func__;

  if (kServicesSupportingRestart().find(*serviceName) ==
      kServicesSupportingRestart().end()) {
    throw std::runtime_error(
        folly::to<std::string>(
            "Failed to restart gracefully. Unsupported service: ",
            *serviceName));
  }

  // In multi-switch mode, match production ordering (AgentExecutor):
  //   Stop:  sw_agent first, then hw_agent(s)
  //   Start: hw_agent(s) first, then sw_agent
  // This handler runs inside sw_agent, so "systemctl stop sw_agent" kills
  // the process that spawned the shell. The systemd unit uses KillMode=process,
  // so only the main PID is killed — the child shell from popen() survives
  // and continues executing the remaining stop/start commands.
  if (FLAGS_multi_switch && *serviceName == kSwAgentTestService) {
    auto switchIndices = getSwitchIndicesImpl(getSw());
    CHECK(!switchIndices.empty())
        << "No switch indices found in multi-switch mode";

    std::string cmd = folly::to<std::string>("systemctl stop ", *serviceName);
    for (auto index : switchIndices) {
      cmd += folly::to<std::string>(
          " ; systemctl stop ", kHwAgentTestServicePrefix, index);
    }
    for (auto index : switchIndices) {
      cmd += folly::to<std::string>(
          " ; systemctl start ", kHwAgentTestServicePrefix, index);
    }
    cmd += folly::to<std::string>(" ; systemctl start ", *serviceName);
    XLOG(INFO) << "Graceful restart cmd: " << cmd;
    runShellCmd(cmd);
    return;
  }

  auto cmd = folly::to<std::string>("systemctl restart ", *serviceName);
  runShellCmd(cmd);
}

void TestThriftHandler::ungracefullyRestartService(
    std::unique_ptr<std::string> serviceName) {
  XLOG(INFO) << __func__;

  // QSFP ungraceful restart can be implemented by:
  // (A) pkill -9 qsfp
  // (B) touch /dev/shm/fboss/qsfp_service/cold_boot_once_qsfp_service,
  //     restart qsfp
  //
  // QSFP does not save any state for transceivers (non xphy platforms).
  // Thus a process crash (simulated by pkill) is able to warmboot and thus does
  // not flap ports.
  //
  // From the testing standpoint, simulating process crash is of interest, and
  // thus this handler is implemented using pkill -9.
  //
  // (B) is a planned cold boot restart and will flap ports, but is not
  // implemented here.
  if (kServicesSupportingRestart().find(*serviceName) ==
      kServicesSupportingRestart().end()) {
    throw std::runtime_error(
        folly::to<std::string>(
            "Failed to restart ungracefully. Unsupported service: ",
            *serviceName));
  }

  auto it = kServiceToProcessName().find(*serviceName);
  if (it == kServiceToProcessName().end()) {
    throw std::runtime_error(
        folly::to<std::string>(
            "No process name mapping for service: ", *serviceName));
  }

  // pkill -9 followed by systemctl restart: pkill alone won't restart the
  // service since the systemd unit has Restart=no.
  // Use ';' (not '&&') so that if pkill kills the current process
  // (e.g. agent), the orphaned shell still executes systemctl restart.
  std::string cmd;
  if (*serviceName == "fboss_sw_agent_test") {
    std::string fileToCreate = "/dev/shm/fboss/warm_boot/cold_boot_once_0";
    cmd = folly::to<std::string>(
        "pkill -9 ",
        it->second,
        " ; touch ",
        fileToCreate,
        " ; systemctl restart ",
        *serviceName);
  } else {
    cmd = folly::to<std::string>(
        "pkill -9 ", it->second, " ; systemctl restart ", *serviceName);
  }
  runShellCmd(cmd);
}

void TestThriftHandler::gracefullyRestartServiceWithDelay(
    std::unique_ptr<std::string> serviceName,
    int32_t delayInSeconds) {
  XLOG(INFO) << __func__;

  if (*serviceName != "fboss_sw_agent_test") {
    throw std::runtime_error(
        folly::to<std::string>(
            "Failed to restart with delay. Unsupported service: ",
            *serviceName));
  }

  auto unitName = folly::to<std::string>(*serviceName, "_delayed_start");
  // Schedule a restart of service after delay seconds
  auto cmd = folly::to<std::string>(
      "systemd-run --on-active=",
      delayInSeconds,
      "s --unit=",
      unitName,
      "systemctl start ",
      *serviceName);
  std::system(cmd.c_str());

  // Then stop wedge_agent immediately
  cmd = folly::to<std::string>("systemctl stop ", *serviceName);
  std::system(cmd.c_str());
}

void TestThriftHandler::addNeighbor(
    int32_t interfaceID,
    std::unique_ptr<BinaryAddress> ip,
    std::unique_ptr<std::string> mac,
    int32_t portID) {
  ensureConfigured(__func__);

  auto neighborIP = toIPAddress(*ip);

  if (!neighborIP.isV6()) {
    throw std::runtime_error(
        folly::to<std::string>(
            "Thrift API addNeighbor supports IPv6 neighbors only. Neighbor to add:",
            neighborIP));
  }

  auto neighborMac = folly::MacAddress::tryFromString(*mac);
  if (!neighborMac.hasValue()) {
    throw std::runtime_error(
        folly::to<std::string>(
            "Thrift API addNeighbor: invalid MAC address provided: ", *mac));
  }

  // To resolve a neighbor, mimic receiving NDP Response from neighbor
  getSw()->getNeighborUpdater()->receivedNdpMineForIntf(
      InterfaceID(interfaceID),
      neighborIP.asV6(),
      neighborMac.value(),
      PortDescriptor(PortID(portID)),
      ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
      0 /* flags */);
}

void TestThriftHandler::setSwitchDrainState(
    cfg::SwitchDrainState switchDrainState) {}

void TestThriftHandler::setSelfHealingLagState(int32_t portId, bool enable) {
  ensureVoqOrFabric(__func__);
  const auto port = getSw()->getState()->getPorts()->getNodeIf(PortID(portId));
  if (!port) {
    throw FbossError("no such port ", portId);
  }
  if (port->getPortType() != cfg::PortType::INTERFACE_PORT) {
    throw FbossError(
        "Can set selfHealingLag on INTERFACE port only. Port: ",
        portId,
        " portType: ",
        apache::thrift::util::enumNameSafe(port->getPortType()));
  }

  if (port->getSelfHealingECMPLagEnable().has_value() &&
      port->getSelfHealingECMPLagEnable().value() == enable) {
    XLOG(DBG2) << __func__ << " port already in SelfHealingLagState"
               << (enable ? "ENABLED" : "DISABLED");
    return;
  }

  auto updateFn = [portId, enable](const std::shared_ptr<SwitchState>& state) {
    const auto oldPort = state->getPorts()->getNodeIf(PortID(portId));
    std::shared_ptr<SwitchState> newState{state};
    auto newPort = oldPort->modify(&newState);
    newPort->setSelfHealingECMPLagEnable(enable);
    return newState;
  };
  getSw()->updateStateBlocking("set Port SelfHealingLagstate", updateFn);
}

void TestThriftHandler::setConditionalEntropyRehash(
    int32_t portId,
    bool enable) {
  ensureVoqOrFabric(__func__);
  const auto port = getSw()->getState()->getPorts()->getNodeIf(PortID(portId));
  if (!port) {
    throw FbossError("no such port ", portId);
  }
  if (port->getPortType() != cfg::PortType::INTERFACE_PORT) {
    throw FbossError(
        "Can set Conditional Entropy Rehash on INTERFACE port only. Port: ",
        portId,
        " portType: ",
        apache::thrift::util::enumNameSafe(port->getPortType()));
  }

  if (port->getConditionalEntropyRehash() == enable) {
    XLOG(DBG2) << __func__ << " port already in conditionalEntroRehash: "
               << (enable ? "ENABLED" : "DISABLED");
    return;
  }

  auto updateFn = [portId, enable](const std::shared_ptr<SwitchState>& state) {
    const auto oldPort = state->getPorts()->getNodeIf(PortID(portId));
    std::shared_ptr<SwitchState> newState{state};
    auto newPort = oldPort->modify(&newState);
    newPort->setConditionalEntropyRehash(enable);
    return newState;
  };
  getSw()->updateStateBlocking("set Port ConditionalEntropyHash", updateFn);
}

} // namespace facebook::fboss
