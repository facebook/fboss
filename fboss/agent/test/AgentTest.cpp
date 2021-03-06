// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentTest.h"
#include "fboss/agent/Main.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

namespace {
int argCount{0};
char** argVec{nullptr};
facebook::fboss::PlatformInitFn initPlatform{nullptr};
} // unnamed namespace

DEFINE_bool(setup_for_warmboot, false, "Set up test for warmboot");
DEFINE_bool(run_forever, false, "run the test forever");

namespace facebook::fboss {

void AgentTest::setupAgent() {
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
  // Cannot join the thread because initAgent starts a thrift server in the main
  // thread and that runs for lifetime of the application.
  asyncInitThread_->detach();
  initializer()->waitForInitDone();
  XLOG(INFO) << "Agent has been setup and ready for the test";
}

void AgentTest::TearDown() {
  if (FLAGS_run_forever) {
    runForever();
  }
  AgentInitializer::stopAgent(FLAGS_setup_for_warmboot);
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

void AgentTest::setPortStatus(PortID portId, bool up) {
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

void AgentTest::setupConfigFlag() {
  // Nothing to setup by default
}

void AgentTest::setupFlags() {
  // Nothing to setup by default
}

AgentTest::~AgentTest() {}

void initAgentTest(int argc, char** argv, PlatformInitFn initPlatformFn) {
  argCount = argc;
  argVec = argv;
  initPlatform = initPlatformFn;
}

} // namespace facebook::fboss
