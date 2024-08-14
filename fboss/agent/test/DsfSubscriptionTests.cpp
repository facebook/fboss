// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AgentFsdbSyncManager.h"
#include "fboss/agent/DsfSubscription.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/fsdb/tests/utils/FsdbTestServer.h"
#include "fboss/lib/CommonUtils.h"

#include <folly/executors/IOThreadPoolExecutor.h>
#include <folly/executors/thread_factory/NamedThreadFactory.h>
#include <gtest/gtest.h>

namespace facebook::fboss {
namespace {
constexpr auto kRemoteSwitchIdBegin = 4;
constexpr auto kSysPortBlockSize = 50;
constexpr auto kSysPortRangeMin = kRemoteSwitchIdBegin * kSysPortBlockSize;
constexpr auto intfV4AddrPrefix = "42.42.42.";
constexpr auto intfV6AddrPrefix = "42::";
std::shared_ptr<SystemPortMap> makeSysPortsForSwitchIds(
    const std::set<SwitchID>& remoteSwitchIds,
    int numSysPorts = 1) {
  auto sysPorts = std::make_shared<SystemPortMap>();
  for (auto switchId : remoteSwitchIds) {
    auto sysPortBegin = switchId * kSysPortBlockSize + 1;
    for (auto sysPortId = sysPortBegin; sysPortId < sysPortBegin + numSysPorts;
         ++sysPortId) {
      sysPorts->addNode(makeSysPort(std::nullopt, sysPortId, switchId));
    }
  }
  return sysPorts;
}
std::shared_ptr<InterfaceMap> makeRifs(const SystemPortMap* sysPorts) {
  auto rifs = std::make_shared<InterfaceMap>();
  for (const auto& [id, sysPort] : *sysPorts) {
    auto rif = std::make_shared<Interface>(
        InterfaceID(id),
        RouterID(0),
        std::optional<VlanID>(std::nullopt),
        folly::StringPiece("rif"),
        folly::MacAddress("01:02:03:04:05:06"),
        9000,
        false,
        true,
        cfg::InterfaceType::SYSTEM_PORT);
    Interface::Addresses addresses{
        {folly::IPAddressV4(
             folly::to<std::string>(intfV4AddrPrefix, (id % 256))),
         31},
        {folly::IPAddressV6(
             folly::to<std::string>(intfV6AddrPrefix, (id % 256))),
         127}};
    rif->setAddresses(addresses);
    rif->setScope(cfg::Scope::GLOBAL);
    rif->setRemoteInterfaceType(RemoteInterfaceType::DYNAMIC_ENTRY);
    rif->setRemoteLivenessStatus(LivenessStatus::LIVE);
    rifs->addNode(rif);
  }
  return rifs;
}
} // namespace

class DsfSubscriptionTest : public ::testing::Test {
 public:
  void SetUp() override {
    FLAGS_publish_state_to_fsdb = true;
    FLAGS_fsdb_sync_full_state = true;
    auto config = testConfigA(cfg::SwitchType::VOQ);
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
    fsdbTestServer_ = std::make_unique<fsdb::test::FsdbTestServer>();
    FLAGS_fsdbPort = fsdbTestServer_->getFsdbPort();
    pubSub_ = std::make_unique<fsdb::FsdbPubSubManager>("test-client");
    streamConnectPool_ = std::make_unique<folly::IOThreadPoolExecutor>(
        1,
        std::make_shared<folly::NamedThreadFactory>(
            "DsfSubscriberStreamConnect"));
    streamServePool_ = std::make_unique<folly::IOThreadPoolExecutor>(
        1,
        std::make_shared<folly::NamedThreadFactory>(
            "DsfSubscriberStreamServe"));
    hwUpdatePool_ = std::make_unique<folly::IOThreadPoolExecutor>(
        1, std::make_shared<folly::NamedThreadFactory>("DsfHwUpdate"));
  }

  void TearDown() override {
    fsdbTestServer_.reset();
    stopPublisher();
  }

  void createPublisher() {
    publisher_ = std::make_unique<AgentFsdbSyncManager>();
    publisher_->start();
  }
  std::set<SwitchID> remoteSwitchIds() const {
    return std::set<SwitchID>({SwitchID(kRemoteSwitchIdBegin)});
  }
  std::shared_ptr<SystemPortMap> makeSysPorts(int numSysPorts = 1) const {
    return makeSysPortsForSwitchIds(remoteSwitchIds(), numSysPorts);
  }

  std::shared_ptr<SwitchState> makeSwitchState() const {
    auto state = std::make_shared<SwitchState>();
    auto sysPorts = std::make_shared<MultiSwitchSystemPortMap>();
    auto sysPortMap = makeSysPorts();
    sysPorts->addMapNode(sysPortMap, matcher());
    state->resetSystemPorts(sysPorts);
    auto intfs = std::make_shared<MultiSwitchInterfaceMap>();
    auto intfMap = makeRifs(sysPortMap.get());
    intfs->addMapNode(intfMap, matcher());
    state->resetIntfs(intfs);
    return state;
  }
  void publishSwitchState(std::shared_ptr<SwitchState> state) {
    CHECK(publisher_);
    publisher_->stateUpdated(StateDelta(std::shared_ptr<SwitchState>(), state));
  }

  void updateDsfSubscriberState(
      const std::string& nodeName,
      fsdb::FsdbSubscriptionState newState) {
    publisher_->updateDsfSubscriberState(
        DsfSubscription::makeRemoteEndpoint(nodeName, folly::IPAddress("::1")),
        fsdb::FsdbSubscriptionState::DISCONNECTED, // old state doesn't matter
        newState);
  }

  void stopPublisher(bool gr = false) {
    if (publisher_) {
      publisher_->stop(gr);
      publisher_.reset();
    }
  }
  std::unique_ptr<DsfSubscription> createSubscription() {
    fsdb::SubscriptionOptions opts{
        "test-sub", false /* subscribeStats */, FLAGS_dsf_gr_hold_time};
    return std::make_unique<DsfSubscription>(
        std::move(opts),
        streamConnectPool_->getEventBase(),
        streamServePool_->getEventBase(),
        hwUpdatePool_->getEventBase(),
        "local",
        "remote",
        std::set<SwitchID>({SwitchID(kRemoteSwitchIdBegin)}),
        folly::IPAddress("::1"),
        folly::IPAddress("::1"),
        sw_);
  }

  HwSwitchMatcher matcher(uint32_t switchID = kRemoteSwitchIdBegin) const {
    return HwSwitchMatcher(std::unordered_set<SwitchID>({SwitchID(switchID)}));
  }

  std::shared_ptr<SystemPortMap> getRemoteSystemPorts() const {
    return sw_->getState()->getRemoteSystemPorts()->getAllNodes();
  }
  std::shared_ptr<InterfaceMap> getRemoteInterfaces() const {
    return sw_->getState()->getRemoteInterfaces()->getAllNodes();
  }
  DsfSessionState dsfSessionState() const {
    return *subscription_->dsfSessionThrift().state();
  }

 protected:
  void verifyRemoteIntfRouteDelta(
      StateDelta delta,
      int expectedRouteAdded,
      int expectedRouteDeleted) {
    auto routesAdded = 0;
    auto routesDeleted = 0;

    for (const auto& routeDelta : delta.getFibsDelta()) {
      DeltaFunctions::forEachChanged(
          routeDelta.getFibDelta<folly::IPAddressV4>(),
          [&](const auto& /*oldNode*/, const auto& /*newNode*/) {},
          [&](const auto& added) {
            EXPECT_TRUE(added->isConnected());
            EXPECT_EQ(added->getID().rfind(intfV4AddrPrefix, 0), 0);
            routesAdded++;
          },
          [&](const auto& /*removed*/) { routesDeleted++; });

      DeltaFunctions::forEachChanged(
          routeDelta.getFibDelta<folly::IPAddressV6>(),
          [&](const auto& /*oldNode*/, const auto& /*newNode*/) {},
          [&](const auto& added) {
            EXPECT_TRUE(added->isConnected());
            EXPECT_EQ(added->getID().rfind(intfV6AddrPrefix, 0), 0);
            routesAdded++;
          },
          [&](const auto& /*removed*/) { routesDeleted++; });
    }

    EXPECT_EQ(routesAdded, expectedRouteAdded);
    EXPECT_EQ(routesDeleted, expectedRouteDeleted);
  }

  std::unique_ptr<fsdb::test::FsdbTestServer> fsdbTestServer_;
  std::unique_ptr<AgentFsdbSyncManager> publisher_;
  std::unique_ptr<fsdb::FsdbPubSubManager> pubSub_;
  std::unique_ptr<folly::IOThreadPoolExecutor> streamConnectPool_;
  std::unique_ptr<folly::IOThreadPoolExecutor> streamServePool_;
  std::unique_ptr<folly::IOThreadPoolExecutor> hwUpdatePool_;
  std::optional<ReconnectingThriftClient::ServerOptions> serverOptions_;
  std::unique_ptr<HwTestHandle> handle_;
  std::shared_ptr<DsfSubscription> subscription_;
  SwSwitch* sw_;
};

TEST_F(DsfSubscriptionTest, Connect) {
  createPublisher();
  auto state = makeSwitchState();
  publishSwitchState(state);
  std::optional<std::map<SwitchID, std::shared_ptr<SystemPortMap>>>
      recvSysPorts;
  std::optional<std::map<SwitchID, std::shared_ptr<InterfaceMap>>> recvIntfs;
  subscription_ = createSubscription();
  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(getRemoteSystemPorts()->size(), 1);
    ASSERT_EVENTUALLY_EQ(getRemoteInterfaces()->size(), 1);
    EXPECT_EQ(dsfSessionState(), DsfSessionState::WAIT_FOR_REMOTE);
  });

  updateDsfSubscriberState("local", fsdb::FsdbSubscriptionState::CONNECTED);
  WITH_RETRIES(
      ASSERT_EVENTUALLY_EQ(dsfSessionState(), DsfSessionState::ESTABLISHED));
}

TEST_F(DsfSubscriptionTest, ConnectDisconnect) {
  createPublisher();
  auto state = makeSwitchState();
  publishSwitchState(state);
  std::optional<std::map<SwitchID, std::shared_ptr<SystemPortMap>>>
      recvSysPorts;
  std::optional<std::map<SwitchID, std::shared_ptr<InterfaceMap>>> recvIntfs;
  subscription_ = createSubscription();

  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(getRemoteSystemPorts()->size(), 1);
    ASSERT_EVENTUALLY_EQ(getRemoteInterfaces()->size(), 1);
    EXPECT_EQ(dsfSessionState(), DsfSessionState::WAIT_FOR_REMOTE);
  });

  stopPublisher();
  EXPECT_EQ(dsfSessionState(), DsfSessionState::CONNECT);
}

TEST_F(DsfSubscriptionTest, GR) {
  createPublisher();
  auto state = makeSwitchState();
  publishSwitchState(state);
  FLAGS_dsf_gr_hold_time = 5;
  subscription_ = createSubscription();

  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(getRemoteSystemPorts()->size(), 1);
    ASSERT_EVENTUALLY_EQ(getRemoteInterfaces()->size(), 1);
    EXPECT_EQ(dsfSessionState(), DsfSessionState::WAIT_FOR_REMOTE);
  });

  stopPublisher(true);
  createPublisher();
  publishSwitchState(state);

  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(getRemoteSystemPorts()->size(), 1);
    ASSERT_EVENTUALLY_EQ(getRemoteInterfaces()->size(), 1);
    ASSERT_EVENTUALLY_EQ(dsfSessionState(), DsfSessionState::WAIT_FOR_REMOTE);
  });

  auto assertStatus = [this](LivenessStatus expectedStatus) {
    auto assertObjStatus = [expectedStatus](const auto& objs) {
      std::for_each(
          objs->begin(), objs->end(), [expectedStatus](const auto& idAndObj) {
            EXPECT_EQ(
                idAndObj.second->getRemoteLivenessStatus(), expectedStatus);
          });
    };
    assertObjStatus(getRemoteSystemPorts());
    assertObjStatus(getRemoteInterfaces());
  };
  CounterCache counters(sw_);
  // Should be LIVE before GR expire
  assertStatus(LivenessStatus::LIVE);
  stopPublisher(true);
  auto grExpiredCounter =
      SwitchStats::kCounterPrefix + "dsfsession_gr_expired.sum.60";
  WITH_RETRIES({
    counters.update();
    ASSERT_EVENTUALLY_EQ(dsfSessionState(), DsfSessionState::CONNECT);
    ASSERT_EVENTUALLY_TRUE(counters.checkExist(grExpiredCounter));
    ASSERT_EVENTUALLY_EQ(counters.value(grExpiredCounter), 1);
  });
  // Should be STATLE after GR expire
  assertStatus(LivenessStatus::STALE);
}

TEST_F(DsfSubscriptionTest, DataUpdate) {
  createPublisher();
  auto state = makeSwitchState();
  publishSwitchState(state);

  std::optional<std::map<SwitchID, std::shared_ptr<SystemPortMap>>>
      recvSysPorts;
  std::optional<std::map<SwitchID, std::shared_ptr<InterfaceMap>>> recvIntfs;
  subscription_ = createSubscription();

  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(getRemoteSystemPorts()->size(), 1);
    ASSERT_EVENTUALLY_EQ(getRemoteInterfaces()->size(), 1);
    EXPECT_EQ(dsfSessionState(), DsfSessionState::WAIT_FOR_REMOTE);
  });

  auto sysPort2 = makeSysPort(
      std::nullopt, SystemPortID(kSysPortRangeMin + 2), kRemoteSwitchIdBegin);
  auto portMap = state->getSystemPorts()->modify(&state);
  portMap->addNode(sysPort2, matcher());
  publishSwitchState(state);

  WITH_RETRIES(ASSERT_EVENTUALLY_EQ(getRemoteSystemPorts()->size(), 2));
}

TEST_F(DsfSubscriptionTest, updateWithRollbackProtection) {
  auto sysPorts = makeSysPorts(2);
  auto rifs = makeRifs(sysPorts.get());

  std::map<SwitchID, std::shared_ptr<SystemPortMap>> switchId2SystemPorts;
  std::map<SwitchID, std::shared_ptr<InterfaceMap>> switchId2Intfs;
  switchId2SystemPorts[SwitchID(kRemoteSwitchIdBegin)] = sysPorts;
  switchId2Intfs[SwitchID(kRemoteSwitchIdBegin)] = rifs;

  // Add remote interfaces
  const auto prevState = sw_->getState();
  subscription_ = createSubscription();
  subscription_->updateWithRollbackProtection(
      switchId2SystemPorts, switchId2Intfs);

  const auto addedState = sw_->getState();
  addedState->publish();
  verifyRemoteIntfRouteDelta(StateDelta(prevState, addedState), 4, 0);

  // Change remote interface routes
  switchId2SystemPorts[SwitchID(kRemoteSwitchIdBegin)] = makeSysPorts(2);
  switchId2Intfs[SwitchID(kRemoteSwitchIdBegin)] = makeRifs(sysPorts.get());

  const auto sysPort1Id = kSysPortRangeMin + 1;
  Interface::Addresses updatedAddresses{
      {folly::IPAddressV4(
           folly::to<std::string>(intfV4AddrPrefix, (sysPort1Id % 256 + 10))),
       31},
      {folly::IPAddressV6(
           folly::to<std::string>(intfV6AddrPrefix, (sysPort1Id % 256 + 10))),
       127}};
  switchId2Intfs[SwitchID(kRemoteSwitchIdBegin)]
      ->find(sysPort1Id)
      ->second->setAddresses(updatedAddresses);

  subscription_->updateWithRollbackProtection(
      switchId2SystemPorts, switchId2Intfs);

  auto modifiedState = sw_->getState();
  verifyRemoteIntfRouteDelta(StateDelta(addedState, modifiedState), 2, 2);

  // Remove remote interface routes
  switchId2SystemPorts[SwitchID(kRemoteSwitchIdBegin)] =
      std::make_shared<SystemPortMap>();
  switchId2Intfs[SwitchID(kRemoteSwitchIdBegin)] =
      std::make_shared<InterfaceMap>();

  subscription_->updateWithRollbackProtection(
      switchId2SystemPorts, switchId2Intfs);

  waitForStateUpdates(sw_);
  auto deletedState = sw_->getState();
  verifyRemoteIntfRouteDelta(StateDelta(addedState, deletedState), 0, 4);
}

TEST_F(DsfSubscriptionTest, setupNeighbors) {
  subscription_ = createSubscription();
  auto updateAndCompareTables = [this](
                                    const auto& sysPorts,
                                    const auto& rifs,
                                    bool publishState,
                                    bool noNeighbors = false) {
    if (publishState) {
      rifs->publish();
    }

    // subscription_->updateWithRollbackProtection is expected to set isLocal
    // to False, and rest of the structure should remain the same.
    auto expectedRifs = InterfaceMap(rifs->toThrift());
    for (auto intfIter : expectedRifs) {
      auto& intf = intfIter.second;
      for (auto& ndpEntry : *intf->getNdpTable()) {
        ndpEntry.second->setIsLocal(false);
        ndpEntry.second->setNoHostRoute(false);
      }
      for (auto& arpEntry : *intf->getArpTable()) {
        arpEntry.second->setIsLocal(false);
        arpEntry.second->setNoHostRoute(false);
      }
    }

    std::map<SwitchID, std::shared_ptr<SystemPortMap>> switchId2SystemPorts;
    std::map<SwitchID, std::shared_ptr<InterfaceMap>> switchId2Intfs;
    switchId2SystemPorts[SwitchID(kRemoteSwitchIdBegin)] = sysPorts;
    switchId2Intfs[SwitchID(kRemoteSwitchIdBegin)] = rifs;

    subscription_->updateWithRollbackProtection(
        switchId2SystemPorts, switchId2Intfs);

    waitForStateUpdates(sw_);
    EXPECT_EQ(
        sysPorts->toThrift(),
        sw_->getState()->getRemoteSystemPorts()->getAllNodes()->toThrift());

    for (const auto& [_, intfMap] :
         std::as_const(*sw_->getState()->getRemoteInterfaces())) {
      for (const auto& [_, localRif] : std::as_const(*intfMap)) {
        const auto& expectedRif = expectedRifs.at(localRif->getID());
        // Since resolved timestamp is only set locally, update expectedRifs to
        // the same timestamp such that they're the same, for both arp and ndp.
        for (const auto& [_, arp] : std::as_const(*localRif->getArpTable())) {
          EXPECT_TRUE(arp->getResolvedSince().has_value());
          if (arp->getResolvedSince().has_value()) {
            expectedRif->getArpTable()
                ->at(arp->getID())
                ->setResolvedSince(*arp->getResolvedSince());
          }
        }
        for (const auto& [_, ndp] : std::as_const(*localRif->getNdpTable())) {
          EXPECT_TRUE(ndp->getResolvedSince().has_value());
          if (ndp->getResolvedSince().has_value()) {
            expectedRif->getNdpTable()
                ->at(ndp->getID())
                ->setResolvedSince(*ndp->getResolvedSince());
          }
        }
      }
    }
    EXPECT_EQ(
        apache::thrift::SimpleJSONSerializer::serialize<std::string>(
            expectedRifs.toThrift()),
        apache::thrift::SimpleJSONSerializer::serialize<std::string>(
            sw_->getState()->getRemoteInterfaces()->getAllNodes()->toThrift()));

    // neighbor entries are modified to set isLocal=false
    // Thus, if neighbor table is non-empty, programmed vs. actually
    // programmed would be unequal for published state.
    // for unpublished state, the passed state would be modified, and thus,
    // programmed vs actually programmed state would be equal.
    EXPECT_TRUE(
        rifs->toThrift() !=
            sw_->getState()->getRemoteInterfaces()->getAllNodes()->toThrift() ||
        noNeighbors || !publishState);
  };

  auto verifySetupNeighbors = [&](bool publishState) {
    {
      // No neighbors
      auto sysPorts = makeSysPorts();
      auto rifs = makeRifs(sysPorts.get());
      updateAndCompareTables(
          sysPorts, rifs, publishState, true /* noNeighbors */);
    }
    {
      // add neighbors
      auto sysPorts = makeSysPorts();
      auto rifs = makeRifs(sysPorts.get());
      auto firstRif = kSysPortRangeMin + 1;
      auto [ndpTable, arpTable] = makeNbrs();
      rifs->ref(firstRif)->setNdpTable(ndpTable);
      rifs->ref(firstRif)->setArpTable(arpTable);
      updateAndCompareTables(sysPorts, rifs, publishState);
    }
    {
      // update neighbors
      auto sysPorts = makeSysPorts();
      auto rifs = makeRifs(sysPorts.get());
      auto firstRif = kSysPortRangeMin + 1;
      auto [ndpTable, arpTable] = makeNbrs();
      ndpTable.begin()->second.mac() = "06:05:04:03:02:01";
      arpTable.begin()->second.mac() = "06:05:04:03:02:01";
      rifs->ref(firstRif)->setNdpTable(ndpTable);
      rifs->ref(firstRif)->setArpTable(arpTable);
      updateAndCompareTables(sysPorts, rifs, publishState);
    }
    {
      // delete neighbors
      auto sysPorts = makeSysPorts();
      auto rifs = makeRifs(sysPorts.get());
      auto firstRif = kSysPortRangeMin + 1;
      auto [ndpTable, arpTable] = makeNbrs();
      ndpTable.erase(ndpTable.begin());
      arpTable.erase(arpTable.begin());
      rifs->ref(firstRif)->setNdpTable(ndpTable);
      rifs->ref(firstRif)->setArpTable(arpTable);
      updateAndCompareTables(sysPorts, rifs, publishState);
    }
    {
      // clear neighbors
      auto sysPorts = makeSysPorts();
      auto rifs = makeRifs(sysPorts.get());
      updateAndCompareTables(
          sysPorts, rifs, publishState, true /* noNeighbors */);
    }
  };

  verifySetupNeighbors(false /* publishState */);
  verifySetupNeighbors(true /* publishState */);
}
} // namespace facebook::fboss
