/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <fb303/ServiceData.h>
#include <folly/Subprocess.h>
#include <neteng/ai/te_agent/if/gen-cpp2/TeAgentService.h>
#include <neteng/ai/te_agent/if/gen-cpp2/TeAgent_types.h>
#include <neteng/ai/te_agent/tests/TestUtils.h>
#include "common/thrift/thrift/gen-cpp2/MonitorAsyncClient.h"
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTeFlowTestUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/integration_tests/AgentIntegrationTest.h"
#include "fboss/lib/CommonUtils.h"
#include "servicerouter/client/cpp2/ClientFactory.h"
#include "servicerouter/client/cpp2/ServiceRouter.h"

namespace {
static constexpr int kTeAgentThriftPort{2022};
static std::string kNhopAddrA("1::1");
static std::string kNhopAddrB("2::2");
static std::string kDstIpStart("103");
static std::string kCounterId1("rtsw001:host1:nic1-rtsw002:host1");
static std::string kCounterId2("rtsw001:host1:nic1-rtsw003:host1");
static int kPrefixLength(59);
static int kTeFlowEntries(9000);
} // namespace

using namespace facebook::neteng::ai;
using namespace std::chrono;

using StateUpdateFn = facebook::fboss::SwSwitch::StateUpdateFn;

namespace facebook::fboss {
class TeAgentIntegrationTest : public AgentIntegrationTest {
 protected:
  void SetUp() override {
    if (!FLAGS_config.empty()) {
      std::cerr << "Modify config to enable EM " << FLAGS_config << std::endl;
      auto agentConfig = AgentConfig::fromFile(FLAGS_config)->thrift;
      // setup EM config here
      auto& bcm = *(agentConfig.platform()->chip()->bcm_ref());
      auto yamlCfg = bcm.yamlConfig();
      if (yamlCfg) {
        std::string emSt("fpem_mem_entries");
        std::size_t pos = (*yamlCfg).find(emSt);
        if (pos == std::string::npos) {
          // use common func
          facebook::fboss::enableExactMatch(*yamlCfg);
        }
      } else {
        auto& cfg = *(bcm.config());
        if (cfg.find("fpem_mem_entries") == cfg.end()) {
          cfg["fpem_mem_entries"] = "0x10000";
        }
      }
      auto newAgentConfig = AgentConfig(
          agentConfig,
          apache::thrift::SimpleJSONSerializer::serialize<std::string>(
              agentConfig));
      newAgentConfig.dumpConfig(FLAGS_config);
    }
    AgentIntegrationTest::SetUp();
  }

  cfg::SwitchConfig initialConfig(const AgentEnsemble& ensemble) override {
    auto cfg = AgentIntegrationTest::initialConfig(ensemble);
    cfg::ExactMatchTableConfig tableConfig;
    tableConfig.name() = "TeFlowTable";
    tableConfig.dstPrefixLength() = 59;
    cfg.switchSettings()->exactMatchTableConfigs() = {tableConfig};
    return cfg;
  }

  uint64_t teAgentAliveSince() {
    uint64_t aliveSince{0};
    auto clientParams = servicerouter::ClientParams();
    clientParams.setSingleHost("::1", kTeAgentThriftPort);
    checkWithRetry(
        [&aliveSince, &clientParams]() {
          try {
            auto client =
                servicerouter::cpp2::getClientFactory()
                    .getSRClientUnique<facebook::thrift::MonitorAsyncClient>(
                        "", clientParams);
            aliveSince = client->sync_aliveSince();
            return true;
          } catch (const std::exception&) {
            return false;
          }
        },
        60, /* retries */
        1s,
        "Cannot connect to TeAgent service");
    return aliveSince;
  }

  void checkTeAgentState() {
    auto aliveSince = teAgentAliveSince();
    EXPECT_NE(aliveSince, 0);
  }

  void syncTeFlows(std::vector<teagent::FlowRoute>& teFlowRoutes) {
    auto clientParams = servicerouter::ClientParams();
    clientParams.setSingleHost("::1", kTeAgentThriftPort);

    std::chrono::milliseconds processingTimeout{60000};
    clientParams.setProcessingTimeoutMs(processingTimeout);
    clientParams.setOverallTimeoutMs(processingTimeout);
    auto teClient =
        servicerouter::cpp2::getClientFactory()
            .getSRClientUnique<apache::thrift::Client<teagent::TeAgentService>>(
                "", clientParams);
    if (!teClient) {
      XLOG(DBG2) << "Failed to connect to te agent";
    }

    auto switchName = getLocalHostname();
    // set flow routes
    teClient->sync_setFlowRoutes(
        std::move(switchName), std::move(teFlowRoutes));
  }

  void resolveNextHops() {
    auto ecmpHelper = std::make_unique<utility::EcmpSetupTargetedPorts6>(
        getSw()->getState(), getSw()->needL2EntryForNeighbor(), RouterID(0));

    auto StateUpdateFn = [&](auto state) {
      return ecmpHelper->resolveNextHops(
          state,
          {PortDescriptor(getAgentEnsemble()->masterLogicalPortIds()[0]),
           PortDescriptor(getAgentEnsemble()->masterLogicalPortIds()[1])});
    };
    getSw()->updateStateBlocking("Resolve nhops", std::move(StateUpdateFn));
  }

  void syncState() {
    getSw()->getNeighborUpdater()->waitForPendingUpdates();
    waitForBackgroundThread(getSw());
    waitForStateUpdates(getSw());
  }

  // Make single FlowRoute
  teagent::FlowRoute makeFlowRoute(
      const std::string& teFlowId,
      const folly::IPAddressV6& nextHop,
      const std::string& nexthopItfName,
      const std::string& srcItfName,
      const std::string& dstIpPrefix,
      int prefixLength) {
    teagent::Flow flow;
    flow.flowID() = teFlowId;
    teagent::FlowPath path;
    path.edges() = std::vector<teagent::Edge>();
    flow.path() = std::move(path);

    teagent::NextHop nh;
    nh.address() = network::toBinaryAddress(nextHop);
    nh.address()->ifName() = nexthopItfName;

    teagent::FlowRoute route;
    route.flow() = std::move(flow);
    route.srcInterfaceName() = srcItfName;
    network::thrift::IPPrefix ipPrefix;
    ipPrefix.prefixAddress() =
        network::toBinaryAddress(folly::IPAddressV6(dstIpPrefix));
    ipPrefix.prefixLength() = prefixLength;
    route.dstPrefix() = std::move(ipPrefix);
    route.nextHops() = {nh};
    route.nexthops() = {std::move(nh)};
    return route;
  }

  // Make 2 FlowRoutes for the test
  std::unique_ptr<std::vector<teagent::FlowRoute>> makeFlowRoutes(
      const std::string& nextHopAdd1,
      const std::string& nextHopAdd2,
      const std::string& itfName1,
      const std::string& itfName2,
      int prefixLength) {
    auto batch = std::make_unique<std::vector<teagent::FlowRoute>>();

    // flow1
    batch->emplace_back(makeFlowRoute(
        kCounterId1,
        folly::IPAddressV6(nextHopAdd2),
        itfName2,
        itfName1,
        "100::",
        prefixLength));

    // flow2
    batch->emplace_back(makeFlowRoute(
        kCounterId2,
        folly::IPAddressV6(nextHopAdd1),
        itfName1,
        itfName2,
        "101::",
        prefixLength));

    return batch;
  }

  // Make FlowRoutes in scale for the test
  std::unique_ptr<std::vector<teagent::FlowRoute>> makeFlowRoutesScale(
      const std::string& dstIpStart,
      const std::string& nextHop,
      const std::string& nexthopItfName,
      const std::string& srcItfName,
      int prefixLength,
      uint32_t numEntries) {
    auto batch = std::make_unique<std::vector<teagent::FlowRoute>>();

    int count{0};
    int i{0};
    int j{0};
    while (++count <= numEntries) {
      std::string dstIpPrefix = fmt::format("{}:{}:{}::", dstIpStart, i, j);
      std::string id = fmt::format("rtsw001:host1:nic1-rtsw003:host{}", count);
      batch->emplace_back(makeFlowRoute(
          id,
          folly::IPAddressV6(nextHop),
          nexthopItfName,
          srcItfName,
          dstIpPrefix,
          prefixLength));
      if (j++ > 255) {
        i++;
        j = 0;
      }
    }

    return batch;
  }
};

TEST_F(TeAgentIntegrationTest, teAgentRunning) {
  auto verify = [&]() { checkTeAgentState(); };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(TeAgentIntegrationTest, addDeleteTeFlows) {
  AgentEnsemble* ensemble = getAgentEnsemble();
  const auto port = ensemble->masterLogicalPortIds()[0];
  auto switchId = ensemble->scopeResolver().scope(port).switchId();
  auto client = ensemble->getHwAgentTestClient(switchId);

  auto ports = getSw()->getState()->getPorts();
  auto ifName0 =
      ports->getNodeIf(PortID(getAgentEnsemble()->masterLogicalPortIds()[0]))
          ->getName();
  auto ifName1 =
      ports->getNodeIf(PortID(getAgentEnsemble()->masterLogicalPortIds()[1]))
          ->getName();

  auto setup = [&]() {
    checkTeAgentState();
    resolveNextHops();
    // Add 2 teflow entries
    syncState();
    auto teFlowRoutes =
        makeFlowRoutes(kNhopAddrA, kNhopAddrB, ifName0, ifName1, kPrefixLength);
    syncTeFlows(*teFlowRoutes);
  };

  auto verify = [&]() {
    auto clientParams = servicerouter::ClientParams();
    clientParams.setSingleHost("::1", kTeAgentThriftPort);

    std::chrono::milliseconds processingTimeout{60000};
    clientParams.setProcessingTimeoutMs(processingTimeout);
    clientParams.setOverallTimeoutMs(processingTimeout);
    auto teClient =
        servicerouter::cpp2::getClientFactory()
            .getSRClientUnique<apache::thrift::Client<teagent::TeAgentService>>(
                "", clientParams);
    if (!teClient) {
      XLOG(DBG2) << "Failed to connect to te agent";
    }

    // Verify 2 teflow entries received by te_agent
    std::unordered_map<std::string, teagent::FlowRoute> flowIDToFlowRoute;
    teClient->sync_getLastReceivedFlowRoutes(flowIDToFlowRoute);
    CHECK_EQ(flowIDToFlowRoute.size(), 2);

    syncState();

    // Verify 2 teflow entries in wedge agent
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(client->sync_getNumTeFlowEntries(), 2);
      EXPECT_EVENTUALLY_TRUE(client->sync_checkSwHwTeFlowMatch(
          getProgrammedState()
              ->getTeFlowTable()
              ->getNodeIf(getTeFlowStr(utility::makeFlowKey(
                  "100::",
                  getAgentEnsemble()->masterLogicalPortIds()[0],
                  kPrefixLength)))
              ->toThrift()));
      EXPECT_EVENTUALLY_TRUE(client->sync_checkSwHwTeFlowMatch(
          getProgrammedState()
              ->getTeFlowTable()
              ->getNodeIf(getTeFlowStr(utility::makeFlowKey(
                  "101::",
                  getAgentEnsemble()->masterLogicalPortIds()[1],
                  kPrefixLength)))
              ->toThrift()));
    });
    resolveNextHops();

    // Delete a flow and verify
    auto teFlowRoutes =
        makeFlowRoutes(kNhopAddrA, kNhopAddrB, ifName0, ifName1, kPrefixLength);
    teFlowRoutes->pop_back();
    syncTeFlows(*teFlowRoutes);

    // Verify 1 teflow entry received by te_agent
    std::unordered_map<std::string, teagent::FlowRoute> flowIDToFlowRoute2;
    teClient->sync_getLastReceivedFlowRoutes(flowIDToFlowRoute2);
    CHECK_EQ(flowIDToFlowRoute2.size(), 1);

    syncState();

    // Verify 1 teflow entry in wedge agent
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(client->sync_getNumTeFlowEntries(), 2);
      EXPECT_EVENTUALLY_TRUE(client->sync_checkSwHwTeFlowMatch(
          getProgrammedState()
              ->getTeFlowTable()
              ->getNodeIf(getTeFlowStr(utility::makeFlowKey(
                  "100::",
                  getAgentEnsemble()->masterLogicalPortIds()[0],
                  kPrefixLength)))
              ->toThrift()));
    });
    // Add 2 teflow entries for warmboot verification
    teFlowRoutes =
        makeFlowRoutes(kNhopAddrA, kNhopAddrB, ifName0, ifName1, kPrefixLength);
    syncTeFlows(*teFlowRoutes);

    syncState();
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(TeAgentIntegrationTest, addTeFlowsScale) {
  auto ports = getSw()->getState()->getPorts();
  AgentEnsemble* ensemble = getAgentEnsemble();
  const auto port = ensemble->masterLogicalPortIds()[0];
  auto switchId = ensemble->scopeResolver().scope(port).switchId();
  auto client = ensemble->getHwAgentTestClient(switchId);

  auto ifName0 =
      ports->getNodeIf(PortID(getAgentEnsemble()->masterLogicalPortIds()[0]))
          ->getName();
  auto ifName1 =
      ports->getNodeIf(PortID(getAgentEnsemble()->masterLogicalPortIds()[1]))
          ->getName();

  auto setup = [&]() {
    checkTeAgentState();
    resolveNextHops();
    syncState();
    // Add 9K teflow entries since in TH4 the flex counter is shared
    // The Max flex counter can be allocated for TeFlow entries is 9216
    auto teFlowRoutes = makeFlowRoutesScale(
        kDstIpStart,
        kNhopAddrA,
        ifName0,
        ifName1,
        kPrefixLength,
        kTeFlowEntries);
    const auto startTs = std::chrono::steady_clock::now();
    syncTeFlows(*teFlowRoutes);
    const auto endTs = std::chrono::steady_clock::now();
    auto elapsedMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTs - startTs);
    XLOG(DBG2) << "Adding 9K TeFlow entries took " << elapsedMs.count()
               << "ms.";
  };

  auto verify = [&]() {
    auto clientParams = servicerouter::ClientParams();
    clientParams.setSingleHost("::1", kTeAgentThriftPort);

    std::chrono::milliseconds processingTimeout{60000};
    clientParams.setProcessingTimeoutMs(processingTimeout);
    clientParams.setOverallTimeoutMs(processingTimeout);
    auto teClient =
        servicerouter::cpp2::getClientFactory()
            .getSRClientUnique<apache::thrift::Client<teagent::TeAgentService>>(
                "", clientParams);
    if (!teClient) {
      XLOG(DBG2) << "Failed to connect to te agent";
    }

    // Verify 9K teflow entries received by te_agent
    std::unordered_map<std::string, teagent::FlowRoute> flowIDToFlowRoute;
    teClient->sync_getLastReceivedFlowRoutes(flowIDToFlowRoute);
    CHECK_EQ(flowIDToFlowRoute.size(), kTeFlowEntries);

    syncState();
    // Verify 9K teflow entries in wedge agent
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(client->sync_getNumTeFlowEntries(), kTeFlowEntries);
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
