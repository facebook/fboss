/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/hw_test/SaiSwitchEnsemble.h"

#include "fboss/agent/SetupThrift.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiLagManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"

#include <folly/io/async/AsyncSignalHandler.h>
#include "fboss/agent/hw/sai/diag/SaiRepl.h"
#include "fboss/agent/hw/sai/hw_test/SaiTestHandler.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatformInit.h"
#include "fboss/agent/test/LinkStateToggler.h"

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/SwitchStats.h"

#include <folly/gen/Base.h>

#include <boost/container/flat_set.hpp>

#include <csignal>
#include <memory>
#include <thread>

DECLARE_int32(thrift_port);
DECLARE_bool(setup_thrift);
DECLARE_string(config);
DECLARE_bool(classid_for_unresolved_routes);

namespace {
using namespace facebook::fboss;
using folly::AsyncSignalHandler;

void initFlagDefaults(const std::map<std::string, std::string>& defaults) {
  for (auto item : defaults) {
    gflags::SetCommandLineOptionWithMode(
        item.first.c_str(), item.second.c_str(), gflags::SET_FLAGS_DEFAULT);
  }
}

class SignalHandler : public AsyncSignalHandler {
 public:
  explicit SignalHandler(folly::EventBase* eventBase)
      : AsyncSignalHandler(eventBase) {
    registerSignalHandler(SIGINT);
    registerSignalHandler(SIGTERM);
  }
  void signalReceived(int /*signum*/) noexcept override {
    auto evb = getEventBase();
    evb->runInEventBaseThread([evb] { evb->terminateLoopSoon(); });
  }

 private:
};

std::unique_ptr<AgentConfig> getAgentConfig() {
  return FLAGS_config.empty() ? AgentConfig::fromDefaultFile()
                              : AgentConfig::fromFile(FLAGS_config);
}
} // namespace

namespace facebook::fboss {

SaiSwitchEnsemble::SaiSwitchEnsemble(
    const HwSwitchEnsemble::Features& featuresDesired)
    : HwSwitchEnsemble(featuresDesired) {}

std::unique_ptr<std::thread> SaiSwitchEnsemble::createThriftThread(
    const SaiSwitch* hwSwitch) {
  return std::make_unique<std::thread>([hwSwitch] {
    FbossEventBase* eventBase =
        new FbossEventBase("SaiSwitchEnsembleSignalHandler");
    auto handler = std::make_shared<SaiTestHandler>(hwSwitch);
    auto server = setupThriftServer(
        *eventBase, handler, {FLAGS_thrift_port}, true /* setupSSL*/);
    SignalHandler signalHandler(eventBase);
    server->serve();
  });
}

std::vector<PortID> SaiSwitchEnsemble::masterLogicalPortIds() const {
  return getPlatform()->masterLogicalPortIds();
}

std::vector<PortID> SaiSwitchEnsemble::getAllPortsInGroup(PortID portID) const {
  return getPlatform()->getAllPortsInGroup(portID);
}

std::vector<FlexPortMode> SaiSwitchEnsemble::getSupportedFlexPortModes() const {
  return getPlatform()->getSupportedFlexPortModes();
}

void SaiSwitchEnsemble::dumpHwCounters() const {
  // TODO once hw shell access is supported
}

std::map<AggregatePortID, HwTrunkStats>
SaiSwitchEnsemble::getLatestAggregatePortStats(
    const std::vector<AggregatePortID>& aggregatePorts) {
  std::map<AggregatePortID, HwTrunkStats> stats;
  for (auto aggregatePort : aggregatePorts) {
    auto lagStats = getHwSwitch()->managerTable()->lagManager().getHwTrunkStats(
        aggregatePort);

    stats.emplace(
        aggregatePort, (lagStats.has_value() ? *lagStats : HwTrunkStats{}));
  }
  return stats;
}

uint64_t SaiSwitchEnsemble::getSdkSwitchId() const {
  return getHwSwitch()->getSaiSwitchId();
}

void SaiSwitchEnsemble::runDiagCommand(
    const std::string& input,
    std::string& output,
    std::optional<SwitchID> /*switchId*/) {
  ClientInformation clientInfo;
  clientInfo.username() = "hw_test";
  clientInfo.hostname() = "hw_test";
  output = diagCmdServer_->diagCmd(
      std::make_unique<fbstring>(input),
      std::make_unique<ClientInformation>(clientInfo));
}

std::map<int64_t, cfg::DsfNode> SaiSwitchEnsemble::dsfNodesFromInputConfig()
    const {
  return *getAgentConfig()->thrift.sw()->dsfNodes();
}

void SaiSwitchEnsemble::init(
    const HwSwitchEnsemble::HwSwitchEnsembleInitInfo& info) {
  auto agentConfig = getAgentConfig();
  if (info.overrideDsfNodes.has_value()) {
    cfg::AgentConfig thrift = agentConfig->thrift;
    thrift.sw()->dsfNodes() = *info.overrideDsfNodes;
    agentConfig = std::make_unique<AgentConfig>(thrift, "");
  }
  initFlagDefaults(*agentConfig->thrift.defaultCommandLineArgs());
  auto platform =
      initSaiPlatform(std::move(agentConfig), getHwSwitchFeatures(), 0);
  if (platform->getAsic()->getAsicVendor() ==
          HwAsic::AsicVendor::ASIC_VENDOR_TAJO ||
      platform->getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
    FLAGS_classid_for_unresolved_routes = true;
  }
  if (platform->getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
    FLAGS_set_classid_for_my_subnet_and_ip_routes = true;
  }
  if (auto tcvr = info.overrideTransceiverInfo) {
    platform->setOverrideTransceiverInfo(*tcvr);
  }
  std::unique_ptr<LinkStateToggler> linkToggler;
  if (haveFeature(HwSwitchEnsemble::LINKSCAN)) {
    linkToggler = std::make_unique<LinkStateToggler>(this);
  }
  std::unique_ptr<std::thread> thriftThread;
  if (FLAGS_setup_thrift) {
    thriftThread =
        createThriftThread(static_cast<SaiSwitch*>(platform->getHwSwitch()));
  }
  setupEnsemble(
      std::make_unique<HwAgent>(std::move(platform)),
      std::move(linkToggler),
      std::move(thriftThread),
      info);
  // TODO - set streamtype correctly based on sdk version from config
  HwAsic* hwAsicTableEntry = getHwAsicTable()->getHwAsicIf(
      getPlatform()->getAsic()->getSwitchId()
          ? SwitchID(*getPlatform()->getAsic()->getSwitchId())
          : SwitchID(0));
  hwAsicTableEntry->setDefaultStreamType(
      getPlatform()->getAsic()->getDefaultStreamType());
  getPlatform()->initLEDs();
  auto hw = static_cast<SaiSwitch*>(getHwSwitch());
  diagShell_ = std::make_unique<DiagShell>(hw);
  diagCmdServer_ = std::make_unique<DiagCmdServer>(hw, diagShell_.get());
}

void SaiSwitchEnsemble::gracefulExit() {
  diagCmdServer_.reset();
  diagShell_.reset();
  HwSwitchEnsemble::gracefulExit();
}
} // namespace facebook::fboss
