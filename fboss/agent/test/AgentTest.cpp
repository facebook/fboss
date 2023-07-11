// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentTest.h"
#include <folly/gen/Base.h>
#include <optional>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/Main.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/qsfp_service/lib/QsfpClient.h"

namespace {
int argCount{0};
char** argVec{nullptr};
facebook::fboss::PlatformInitFn initPlatform{nullptr};
std::optional<facebook::fboss::cfg::StreamType> streamTypeOpt{std::nullopt};
} // unnamed namespace

DEFINE_bool(setup_for_warmboot, false, "Set up test for warmboot");
DEFINE_bool(run_forever, false, "run the test forever");
DEFINE_bool(run_forever_on_failure, false, "run the test forever on failure");

DECLARE_string(config);

namespace facebook::fboss {

void AgentTest::setupAgent() {
  MonolithicAgentInitializer::createSwitch(
      argCount,
      argVec,
      (HwSwitch::FeaturesDesired::PACKET_RX_DESIRED |
       HwSwitch::FeaturesDesired::LINKSCAN_DESIRED),
      initPlatform);
  if (streamTypeOpt.has_value()) {
    HwAsic* hwAsicTableEntry = sw()->getHwAsicTable()->getHwAsicIf(
        sw()->getPlatform_DEPRECATED()->getAsic()->getSwitchId()
            ? SwitchID(
                  *sw()->getPlatform_DEPRECATED()->getAsic()->getSwitchId())
            : SwitchID(0));
    hwAsicTableEntry->setDefaultStreamType(streamTypeOpt.value());
  }
  FLAGS_verify_apply_oper_delta = true;
  utilCreateDir(getAgentTestDir());
  setupConfigFlag();
  asyncInitThread_.reset(
      new std::thread([this] { MonolithicAgentInitializer::initAgent(); }));
  // Cannot join the thread because initAgent starts a thrift server in the main
  // thread and that runs for lifetime of the application.
  asyncInitThread_->detach();
  initializer()->waitForInitDone();
  XLOG(DBG2) << "Agent has been setup and ready for the test";
}

void AgentTest::TearDown() {
  if (FLAGS_run_forever ||
      (::testing::Test::HasFailure() && FLAGS_run_forever_on_failure)) {
    runForever();
  }
  MonolithicAgentInitializer::stopAgent(FLAGS_setup_for_warmboot);
}

std::map<std::string, HwPortStats> AgentTest::getPortStats(
    const std::vector<std::string>& ports) const {
  auto allPortStats = sw()->getHw_DEPRECATED()->getPortStats();
  std::map<std::string, HwPortStats> portStats;
  std::for_each(ports.begin(), ports.end(), [&](const auto& portName) {
    portStats.insert({portName, allPortStats[portName]});
  });
  return portStats;
}

void AgentTest::resolveNeighbor(
    PortDescriptor portDesc,
    const folly::IPAddress& ip,
    folly::MacAddress mac) {
  // TODO support agg ports as well.
  CHECK(portDesc.isPhysicalPort());
  auto port = sw()->getState()->getPorts()->getNodeIf(portDesc.phyPortID());
  auto vlan = port->getVlans().begin()->first;
  if (ip.isV4()) {
    resolveNeighbor(portDesc, ip.asV4(), vlan, mac);
  } else {
    resolveNeighbor(portDesc, ip.asV6(), vlan, mac);
  }
}

template <typename AddrT>
void AgentTest::resolveNeighbor(
    PortDescriptor port,
    const AddrT& ip,
    VlanID vlanId,
    folly::MacAddress mac) {
  auto resolveNeighborFn = [=](const std::shared_ptr<SwitchState>& state) {
    auto outputState{state->clone()};
    auto vlan = outputState->getVlans()->getNode(vlanId);
    auto nbrTable = vlan->template getNeighborEntryTable<AddrT>()->modify(
        vlanId, &outputState);
    if (nbrTable->getEntryIf(ip)) {
      nbrTable->updateEntry(
          ip, mac, port, vlan->getInterfaceID(), NeighborState::REACHABLE);
    } else {
      nbrTable->addEntry(ip, mac, port, vlan->getInterfaceID());
    }
    return outputState;
  };
  sw()->updateStateBlocking("resolve nbr", resolveNeighborFn);
}

void AgentTest::runForever() const {
  XLOG(DBG2) << "AgentTest run forever...";
  while (true) {
    sleep(1);
    XLOG_EVERY_MS(DBG2, 5000) << "AgentTest running forever";
  }
}

bool AgentTest::waitForSwitchStateCondition(
    std::function<bool(const std::shared_ptr<SwitchState>&)> conditionFn,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry) const {
  auto newState = sw()->getState();
  while (retries--) {
    if (conditionFn(newState)) {
      return true;
    }
    std::this_thread::sleep_for(msBetweenRetry);
    newState = sw()->getState();
  }
  XLOG(DBG3) << "Awaited state condition was never satisfied";
  return false;
}

void AgentTest::setPortStatus(PortID portId, bool up) {
  auto configFnLinkDown = [=](const std::shared_ptr<SwitchState>& state) {
    auto newState = state->clone();
    auto ports = newState->getPorts()->modify(&newState);
    auto port = ports->getNodeIf(portId)->clone();
    port->setAdminState(
        up ? cfg::PortState::ENABLED : cfg::PortState::DISABLED);
    ports->updateNode(port, sw()->getScopeResolver()->scope(port));
    return newState;
  };
  sw()->updateStateBlocking("set port state", configFnLinkDown);
}

void AgentTest::setPortLoopbackMode(PortID portId, cfg::PortLoopbackMode mode) {
  auto setLbMode = [=](const std::shared_ptr<SwitchState>& state) {
    auto newState = state->clone();
    auto ports = newState->getPorts()->modify(&newState);
    auto port = ports->getNodeIf(portId)->clone();
    port->setLoopbackMode(mode);
    return newState;
  };
  sw()->updateStateBlocking("set port loopback mode", setLbMode);
}

// Returns the port names for a given list of portIDs
std::vector<std::string> AgentTest::getPortNames(
    const std::vector<PortID>& ports) const {
  return folly::gen::from(ports) | folly::gen::map([&](PortID port) {
           return sw()->getState()->getPorts()->getNodeIf(port)->getName();
         }) |
      folly::gen::as<std::vector<std::string>>();
}

// Waits till the link status of the passed in ports reaches
// the expected state
void AgentTest::waitForLinkStatus(
    const std::vector<PortID>& portsToCheck,
    bool up,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry) const {
  XLOG(DBG2) << "Checking link status on "
             << folly::join(",", getPortNames(portsToCheck));
  auto portStatus = sw()->getPortStatus();
  std::vector<PortID> badPorts;
  while (retries--) {
    badPorts.clear();
    for (const auto& port : portsToCheck) {
      if (*portStatus[port].up() != up) {
        std::this_thread::sleep_for(msBetweenRetry);
        portStatus = sw()->getPortStatus();
        badPorts.push_back(port);
      }
    }
    if (badPorts.empty()) {
      return;
    }
  }

  auto msg = folly::format(
      "Unexpected Link status {:d} for {:s}",
      !up,
      folly::join(",", getPortNames(badPorts)));
  logLinkDbgMessage(badPorts);
  throw FbossError(msg);
}

void AgentTest::setupConfigFlag() {
  // Nothing to setup by default
}

void AgentTest::assertNoInDiscards(int maxNumDiscards) {
  // When port stat is not updated yet post warmboot (collect timestamp will be
  // -1), retry another round on all ports.
  int numRounds = 0;
  int maxRetry = 5;
  while (numRounds < 2 && maxRetry-- > 0) {
    bool retry = false;
    auto portStats = sw()->getHw_DEPRECATED()->getPortStats();
    for (auto [port, stats] : portStats) {
      auto inDiscards = *stats.inDiscards_();
      XLOG(DBG2) << "Port: " << port << " in discards: " << inDiscards
                 << " in bytes: " << *stats.inBytes_()
                 << " out bytes: " << *stats.outBytes_() << " at timestamp "
                 << *stats.timestamp_();
      if (*stats.timestamp_() > 0) {
        EXPECT_LE(inDiscards, maxNumDiscards);
      } else {
        retry = true;
        break;
      }
    }
    numRounds = retry ? numRounds : numRounds + 1;
    // Allow for a few rounds of stat collection
    sleep(1);
  }
}

void AgentTest::dumpRunningConfig(const std::string& targetDir) {
  cfg::AgentConfig testConfig;
  testConfig.sw() = sw()->getConfig();
  const auto& baseConfig = platform()->config();
  testConfig.platform() = *baseConfig->thrift.platform();
  auto newcfg = AgentConfig(
      testConfig,
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(testConfig));
  newcfg.dumpConfig(targetDir);
}

void AgentTest::setupConfigFile(
    const cfg::SwitchConfig& cfg,
    const std::string& testConfigDir,
    const std::string& configFile) const {
  cfg::AgentConfig testConfig;
  *testConfig.sw() = cfg;

  const auto& baseConfig = platform()->config();
  *testConfig.platform() = *baseConfig->thrift.platform();
  auto newcfg = AgentConfig(
      testConfig,
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(testConfig));
  auto newCfgFile = testConfigDir + "/" + configFile;
  // TODO check if file exists and don't write if it does
  utilCreateDir(testConfigDir);
  newcfg.dumpConfig(newCfgFile);
  FLAGS_config = newCfgFile;
}

void AgentTest::reloadConfig(std::string reason) const {
  // reload config so that test config is loaded
  sw()->applyConfig(reason, true);
}

AgentTest::~AgentTest() {}

SwitchID AgentTest::scope(
    const boost::container::flat_set<PortDescriptor>& ports) {
  return scope(sw()->getState(), ports);
}

SwitchID AgentTest::scope(
    const std::shared_ptr<SwitchState>& state,
    const boost::container::flat_set<PortDescriptor>& ports) {
  auto matcher = sw()->getScopeResolver()->scope(state, ports);
  return matcher.switchId();
}

PortID AgentTest::getPortID(const std::string& portName) const {
  for (auto portMap : std::as_const(*sw()->getState()->getPorts())) {
    for (auto port : std::as_const(*portMap.second)) {
      if (port.second->getName() == portName) {
        return port.second->getID();
      }
    }
  }
  throw FbossError("No port named: ", portName);
}

void initAgentTest(
    int argc,
    char** argv,
    PlatformInitFn initPlatformFn,
    std::optional<cfg::StreamType> streamType) {
  argCount = argc;
  argVec = argv;
  initPlatform = initPlatformFn;
  streamTypeOpt = streamType;
}

} // namespace facebook::fboss
