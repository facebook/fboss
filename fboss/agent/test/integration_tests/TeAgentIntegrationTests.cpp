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
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTeFlowTestUtils.h"
#include "fboss/agent/hw/test/HwTestTeFlowUtils.h"
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
static std::string kIfName1("eth1/25/1");
static std::string kIfName2("eth1/26/1");
static std::string kDstIpStart("103");
static std::string kCounterId1("counterId1");
static std::string kCounterId2("counterId2");
static int kPrefixLength(59);
static int kTeFlowEntries(10000);
} // namespace

using namespace facebook::neteng::ai;

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

  cfg::SwitchConfig initialConfig() const override {
    auto cfg = AgentIntegrationTest::initialConfig();
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
          } catch (const std::exception& e) {
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
        sw()->getState(), RouterID(0));

    sw()->updateStateBlocking("Resolve nhops", [&](auto state) {
      return ecmpHelper->resolveNextHops(
          state,
          {PortDescriptor(masterLogicalPortIds()[0]),
           PortDescriptor(masterLogicalPortIds()[1])});
    });
  }

  void syncState() {
    sw()->getNeighborUpdater()->waitForPendingUpdates();
    waitForBackgroundThread(sw());
    waitForStateUpdates(sw());
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
    route.nextHops()->emplace_back(std::move(nh));
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
      std::string id = fmt::format("counterId{}", count);
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
  verifyAcrossWarmBoots(verify);
}

TEST_F(TeAgentIntegrationTest, addDeleteTeFlows) {
  auto setup = [&]() {
    checkTeAgentState();
    resolveNextHops();
    // Add 2 teflow entries
    syncState();
    auto teFlowRoutes = makeFlowRoutes(
        kNhopAddrA, kNhopAddrB, kIfName1, kIfName2, kPrefixLength);
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
      EXPECT_EVENTUALLY_EQ(utility::getNumTeFlowEntries(sw()->getHw()), 2);
    });
    utility::checkSwHwTeFlowMatch(
        sw()->getHw(),
        sw()->getState(),
        utility::makeFlowKey(
            "100::", masterLogicalPortIds()[0], kPrefixLength));
    utility::checkSwHwTeFlowMatch(
        sw()->getHw(),
        sw()->getState(),
        utility::makeFlowKey(
            "101::", masterLogicalPortIds()[1], kPrefixLength));

    resolveNextHops();

    // Delete a flow and verify
    auto teFlowRoutes = makeFlowRoutes(
        kNhopAddrA, kNhopAddrB, kIfName1, kIfName2, kPrefixLength);
    teFlowRoutes->pop_back();
    syncTeFlows(*teFlowRoutes);

    // Verify 1 teflow entry received by te_agent
    std::unordered_map<std::string, teagent::FlowRoute> flowIDToFlowRoute2;
    teClient->sync_getLastReceivedFlowRoutes(flowIDToFlowRoute2);
    CHECK_EQ(flowIDToFlowRoute2.size(), 1);

    syncState();

    // Verify 1 teflow entry in wedge agent
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(utility::getNumTeFlowEntries(sw()->getHw()), 1);
    });
    utility::checkSwHwTeFlowMatch(
        sw()->getHw(),
        sw()->getState(),
        utility::makeFlowKey(
            "100::", masterLogicalPortIds()[0], kPrefixLength));

    // Add 2 teflow entries for warmboot verification
    teFlowRoutes = makeFlowRoutes(
        kNhopAddrA, kNhopAddrB, kIfName1, kIfName2, kPrefixLength);
    syncTeFlows(*teFlowRoutes);

    syncState();
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(TeAgentIntegrationTest, addTeFlowsScale) {
  auto setup = [&]() {
    checkTeAgentState();
    resolveNextHops();
    syncState();
    // Add 10K teflow entries
    auto teFlowRoutes = makeFlowRoutesScale(
        kDstIpStart,
        kNhopAddrA,
        kIfName1,
        kIfName2,
        kPrefixLength,
        kTeFlowEntries);
    const auto startTs = std::chrono::steady_clock::now();
    syncTeFlows(*teFlowRoutes);
    const auto endTs = std::chrono::steady_clock::now();
    auto elapsedMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTs - startTs);
    XLOG(DBG2) << "Adding 10K TeFlow entries took " << elapsedMs.count()
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

    // Verify 10K teflow entries received by te_agent
    std::unordered_map<std::string, teagent::FlowRoute> flowIDToFlowRoute;
    teClient->sync_getLastReceivedFlowRoutes(flowIDToFlowRoute);
    CHECK_EQ(flowIDToFlowRoute.size(), kTeFlowEntries);

    syncState();
    // Verify 10K teflow entries in wedge agent
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(
          utility::getNumTeFlowEntries(sw()->getHw()), kTeFlowEntries);
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
