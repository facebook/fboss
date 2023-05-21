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

#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include "common/network/NetworkUtil.h"
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/MultiNodeTest.h"
#include "fboss/lib/CommonUtils.h"

using namespace facebook::fboss;
using namespace apache::thrift;

DEFINE_string(
    multiNodeTestRemoteSwitchName,
    "",
    "multinode test remote switch name");
DECLARE_bool(run_forever);
DECLARE_bool(nodeZ);
DEFINE_string(
    multiNodeTestPorts,
    "",
    "Comma separated list of ports for multinode tests");

namespace facebook::fboss {

// Construct a config file by combining the hw config passed
// and sw configs from test utility and set config flag to
// point to the test config
void MultiNodeTest::setupConfigFlag() {
  const auto& baseConfig = platform()->config();

  // Fill in PortMap from baseConfig
  auto pMap = std::make_shared<MultiSwitchPortMap>();
  const auto& swConfig = *baseConfig->thrift.sw();
  for (const auto& portCfg : *swConfig.ports()) {
    state::PortFields portFields;
    portFields.portId() = *portCfg.logicalID();
    portFields.portName() = portCfg.name().value_or({});
    auto port = std::make_shared<Port>(portFields);
    port->setSpeed(*portCfg.speed());
    port->setProfileId(*portCfg.profileID());
    pMap->addNode(port, sw()->getScopeResolver()->scope(port));
  }
  utility::setPortToDefaultProfileIDMap(pMap, platform());
  parseTestPorts(FLAGS_multiNodeTestPorts);
  auto testConfigDir = platform()->getPersistentStateDir() + "/multinode_test/";
  auto newCfgFile = "agent_multinode_test.conf";

  setupConfigFile(initialConfig(), testConfigDir, newCfgFile);

  // reload config so that test config is loaded
  platform()->reloadConfig();
}

void MultiNodeTest::parseTestPorts(std::string portList) {
  if (!portList.empty()) {
    std::vector<std::string> strs;
    folly::split(',', portList, strs, true);
    for (const auto& str : strs) {
      uint32_t portId = folly::to<uint32_t>(str);
      if (portId) {
        testPorts_.emplace_back(portId);
      }
    }
  }
}

void MultiNodeTest::SetUp() {
  AgentTest::SetUp();

  if (isDUT()) {
    // Tune this if certain tests want to WB while keeping some ports down
    waitForLinkStatus(testPorts(), true /*up*/);
  }

  XLOG(DBG0) << "Multinode setup ready";
}

void MultiNodeTest::checkNeighborResolved(
    const folly::IPAddress& ip,
    VlanID vlanId,
    PortDescriptor port) const {
  auto checkNeighbor = [&](auto neighborEntry) {
    EXPECT_NE(neighborEntry, nullptr);
    EXPECT_EQ(neighborEntry->isPending(), false);
    EXPECT_EQ(neighborEntry->getPort(), port);
  };
  if (ip.isV4()) {
    checkNeighbor(sw()->getState()
                      ->getVlans()
                      ->getVlanIf(vlanId)
                      ->getArpTable()
                      ->getEntryIf(ip.asV4()));
  } else {
    checkNeighbor(sw()->getState()
                      ->getVlans()
                      ->getVlanIf(vlanId)
                      ->getNdpTable()
                      ->getEntryIf(ip.asV6()));
  }
}

std::unique_ptr<FbossCtrlAsyncClient> MultiNodeTest::getRemoteThriftClient()
    const {
  folly::EventBase* eb = folly::EventBaseManager::get()->getEventBase();
  auto remoteSwitchIp = facebook::network::NetworkUtil::getHostByName(
      FLAGS_multiNodeTestRemoteSwitchName);
  folly::SocketAddress agent(remoteSwitchIp, 5909);
  auto socket = folly::AsyncSocket::newSocket(eb, agent);
  auto chan = HeaderClientChannel::newChannel(std::move(socket));
  return std::make_unique<FbossCtrlAsyncClient>(std::move(chan));
}

bool MultiNodeTest::isDUT() const {
  return !FLAGS_nodeZ;
}

std::vector<PortID> MultiNodeTest::testPorts() const {
  return testPorts_;
}

std::vector<std::string> MultiNodeTest::testPortNames() const {
  return getPortNames(testPorts());
}

std::map<PortID, PortID> MultiNodeTest::localToRemotePort() const {
  std::map<PortID, PortID> localToRemote;
  auto getNbrs = [&localToRemote, this]() {
    try {
      auto localPorts = testPorts();
      std::sort(localPorts.begin(), localPorts.end());
      std::vector<LinkNeighborThrift> remoteLldpNeighbors;
      getRemoteThriftClient()->sync_getLldpNeighbors(remoteLldpNeighbors);
      for (const auto& nbr : remoteLldpNeighbors) {
        auto localPort = sw()->getState()
                             ->getMultiSwitchPorts()
                             ->getPort(*nbr.printablePortId())
                             ->getID();
        if (std::find(localPorts.begin(), localPorts.end(), localPort) !=
            localPorts.end()) {
          localToRemote[localPort] = *nbr.localPort();
        }
      }
      return localToRemote.size() == testPorts().size();
    } catch (const std::exception& ex) {
      XLOG(DBG2) << "Failed to get nbrs: " << ex.what();
      return false;
    }
  };

  checkWithRetry(getNbrs);
  return localToRemote;
}

int mulitNodeTestMain(int argc, char** argv, PlatformInitFn initPlatformFn) {
  ::testing::InitGoogleTest(&argc, argv);
  initAgentTest(argc, argv, initPlatformFn);
  return RUN_ALL_TESTS();
}

} // namespace facebook::fboss
