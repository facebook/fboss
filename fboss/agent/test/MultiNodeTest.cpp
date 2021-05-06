/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/String.h>
#include <gflags/gflags.h>

#include "common/network/NetworkUtil.h"
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/platforms/wedge/WedgePlatformInit.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/MultiNodeTest.h"

using namespace facebook::fboss;
using namespace apache::thrift;

DEFINE_string(
    multiNodeTestRemoteSwitchName,
    "",
    "multinode test remote switch name");
DEFINE_bool(setup_for_warmboot, false, "Set up test for warmboot");
DEFINE_bool(run_forever, false, "run the test forever");

namespace {
int argCount{0};
char** argVec{nullptr};
PlatformInitFn initPlatform{nullptr};
} // unnamed namespace

namespace facebook::fboss {

void MultiNodeTest::TearDown() {
  stopAgent(FLAGS_setup_for_warmboot);
}

// Construct a config file by combining the hw config passed
// and sw configs from test utility and set config flag to
// point to the test config
void MultiNodeTest::setupConfigFlag() {
  cfg::AgentConfig testConfig;
  utility::setPortToDefaultProfileIDMap(
      std::make_shared<PortMap>(), platform());
  *testConfig.sw_ref() = initialConfig();
  const auto& baseConfig = platform()->config();
  *testConfig.platform_ref() = *baseConfig->thrift.platform_ref();
  auto newcfg = AgentConfig(
      testConfig,
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(testConfig));
  auto testConfigDir = platform()->getPersistentStateDir() + "/multinode_test/";
  auto newCfgFile = testConfigDir + "/agent_multinode_test.conf";
  utilCreateDir(testConfigDir);
  newcfg.dumpConfig(newCfgFile);
  FLAGS_config = newCfgFile;
  // reload config so that test config is loaded
  platform()->reloadConfig();
}

void MultiNodeTest::setupFlags() {
  // no common flag to set yet
}

void MultiNodeTest::SetUp() {
  AgentInitializer::createSwitch(
      argCount,
      argVec,
      (HwSwitch::FeaturesDesired::PACKET_RX_DESIRED |
       HwSwitch::FeaturesDesired::LINKSCAN_DESIRED),
      initPlatform);
  setupConfigFlag();
  setupFlags();
  asyncInitThread_.reset(
      new std::thread([this] { AgentInitializer::initAgent(); }));
  asyncInitThread_->detach();
  initializer()->waitForInitDone();
  XLOG(DBG0) << "Multinode setup ready";
}

std::unique_ptr<FbossCtrlAsyncClient> MultiNodeTest::getRemoteThriftClient() {
  folly::EventBase* eb = folly::EventBaseManager::get()->getEventBase();
  auto remoteSwitchIp = facebook::network::NetworkUtil::getHostByName(
      FLAGS_multiNodeTestRemoteSwitchName);
  folly::SocketAddress agent(remoteSwitchIp, 5909);
  auto socket = folly::AsyncSocket::newSocket(eb, agent);
  auto chan = HeaderClientChannel::newChannel(std::move(socket));
  return std::make_unique<FbossCtrlAsyncClient>(std::move(chan));
}

bool MultiNodeTest::waitForSwitchStateCondition(
    std::function<bool(const std::shared_ptr<SwitchState>&)> conditionFn,
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry) {
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

void MultiNodeTest::setPortStatus(PortID portId, bool up) {
  auto configFnLinkDown = [=](const std::shared_ptr<SwitchState>& state) {
    auto newState = state->clone();
    auto ports = newState->getPorts()->clone();
    auto port = ports->getPort(portId)->clone();
    port->setAdminState(
        up ? cfg::PortState::ENABLED : cfg::PortState::DISABLED);
    ports->updateNode(port);
    newState->resetPorts(ports);
    return newState;
  };
  sw()->updateStateBlocking("set port state", configFnLinkDown);
}

void MultiNodeTest::checkForRemoteSideRun() {
  // Do not tear down setup if run_forver flag is true. This is used on
  // remote side of device under test to keep state running till DUT
  // completes tests. Test will be terminated by the starting script
  // by sending a SIGTERM.
  if (FLAGS_run_forever) {
    XLOG(DBG2) << "MultiNodeLacpTest run forever...";
    while (true) {
      sleep(1);
      XLOG_EVERY_MS(DBG2, 5000) << "MultiNodeLacpTest running forever";
    }
  }
}

int mulitNodeTestMain(int argc, char** argv, PlatformInitFn initPlatformFn) {
  ::testing::InitGoogleTest(&argc, argv);
  argCount = argc;
  argVec = argv;
  initPlatform = initPlatformFn;
  return RUN_ALL_TESTS();
}

} // namespace facebook::fboss
