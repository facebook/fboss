// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AgentFsdbSyncManager.h"
#include "fboss/agent/DsfSubscription.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/fsdb/tests/utils/FsdbTestServer.h"
#include "fboss/lib/CommonUtils.h"

#include <folly/executors/IOThreadPoolExecutor.h>
#include <folly/executors/thread_factory/NamedThreadFactory.h>
#include <folly/synchronization/Baton.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::Return;
namespace facebook::fboss {
namespace {
constexpr auto kRemoteSwitchIdBegin = 4;
constexpr auto kSwitchIdGap = 4;
constexpr auto kSysPortBlockSize = 50;
constexpr auto kSysPortRangeMin =
    (kRemoteSwitchIdBegin / kSwitchIdGap) * kSysPortBlockSize;
constexpr auto intfV4AddrPrefix = "42.42.42.";
constexpr auto intfV6AddrPrefix = "42::";
constexpr auto kDynamicSysPortsOffset = 2;
std::shared_ptr<SystemPortMap> makeSysPortsForSwitchIds(
    const std::set<SwitchID>& remoteSwitchIds,
    int numSysPorts = 1) {
  auto sysPorts = std::make_shared<SystemPortMap>();
  for (auto switchId : remoteSwitchIds) {
    auto sysPortBegin =
        (switchId / kSwitchIdGap) * kSysPortBlockSize + kDynamicSysPortsOffset;
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
    folly::IPAddress ipv4(folly::to<std::string>(intfV4AddrPrefix, (id % 256)));
    folly::IPAddress ipv6(folly::to<std::string>(intfV6AddrPrefix, (id % 256)));
    Interface::Addresses addresses{{ipv4.asV4(), 31}, {ipv6.asV6(), 127}};
    state::NeighborEntries ndpTable, arpTable;
    for (auto isV6 : std::vector<bool>({true, false})) {
      state::NeighborEntryFields nbr;
      nbr.mac() = "01:02:03:04:05:06";
      cfg::PortDescriptor port;
      port.portId() = id;
      port.portType() = cfg::PortDescriptorType::SystemPort;
      nbr.portId() = port;
      nbr.interfaceId() = id;
      nbr.isLocal() = true;
      std::string ip = isV6 ? ipv6.str() : ipv4.str();
      nbr.ipaddress() = ip;
      if (isV6) {
        ndpTable.insert({ip, nbr});
      } else {
        arpTable.insert({ip, nbr});
      }
    }
    rif->setNdpTable(ndpTable);
    rif->setArpTable(arpTable);
    rif->setAddresses(addresses);
    rif->setScope(cfg::Scope::GLOBAL);
    rif->setRemoteInterfaceType(RemoteInterfaceType::DYNAMIC_ENTRY);
    rif->setRemoteLivenessStatus(LivenessStatus::LIVE);
    rifs->addNode(rif);
  }
  return rifs;
}
} // namespace

template <uint16_t NumRemoteAsics, bool SubscribePatch>
struct TestParams {
  static auto constexpr kNumRemoteAsics = NumRemoteAsics;
  static auto constexpr kSubscribePatch = SubscribePatch;
};
using TestTypes = ::testing::Types<
    TestParams<1, true>,
    TestParams<1, false>,
    TestParams<2, true>,
    TestParams<2, false>>;

template <typename TestParam>
class DsfSubscriptionTest : public ::testing::Test {
 public:
  static auto constexpr kNumRemoteSwitchAsics = TestParam::kNumRemoteAsics;
  static auto constexpr kSubscribePatch = TestParam::kSubscribePatch;
  void SetUp() override {
    FLAGS_publish_state_to_fsdb = true;
    FLAGS_fsdb_sync_full_state = true;
    FLAGS_dsf_subscribe = false;
    FLAGS_dsf_subscribe_patch = kSubscribePatch;
    auto config = initialConfig();
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
    fsdbTestServer_ = std::make_unique<fsdb::test::FsdbTestServer>();
    FLAGS_fsdbPort_high_priority = fsdbTestServer_->getFsdbPort();
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

  cfg::SwitchConfig initialConfig() {
    auto config = testConfigA(cfg::SwitchType::VOQ);
    for (auto remoteSwitch : remoteSwitchIds()) {
      int remoteSwitchId = static_cast<int64_t>(remoteSwitch);
      auto dsfNode = makeDsfNodeCfg(remoteSwitchId);
      cfg::Range64 sysPortRange;
      sysPortRange.minimum() =
          remoteSwitchId / kSwitchIdGap * kSysPortBlockSize;
      sysPortRange.maximum() = *sysPortRange.minimum() + kSysPortBlockSize;
      dsfNode.systemPortRanges()->systemPortRanges()->push_back(sysPortRange);
      dsfNode.loopbackIps() = {"::1/128", "169.254.0.1/24"};
      config.dsfNodes()->insert(std::make_pair(remoteSwitchId, dsfNode));
    }
    return config;
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
    std::set<SwitchID> remoteSwitchIds;
    for (auto i = 0; i < kNumRemoteSwitchAsics; ++i) {
      remoteSwitchIds.insert(SwitchID(kRemoteSwitchIdBegin + i * kSwitchIdGap));
    }
    return remoteSwitchIds;
  }
  std::shared_ptr<SystemPortMap> makeSysPorts(int numSysPorts = 1) const {
    return makeSysPortsForSwitchIds(remoteSwitchIds(), numSysPorts);
  }

  std::shared_ptr<SwitchState> makeSwitchState() const {
    auto state = std::make_shared<SwitchState>();
    auto sysPorts = std::make_shared<MultiSwitchSystemPortMap>();
    auto intfs = std::make_shared<MultiSwitchInterfaceMap>();
    for (auto remoteSwitchId : remoteSwitchIds()) {
      auto sysPortMap = makeSysPortsForSwitchIds({remoteSwitchId});
      sysPorts->addMapNode(sysPortMap, matcher(remoteSwitchId));
      auto intfMap = makeRifs(sysPortMap.get());
      intfs->addMapNode(intfMap, matcher(remoteSwitchId));
    }
    state->resetSystemPorts(sysPorts);
    state->resetIntfs(intfs);
    return state;
  }
  std::shared_ptr<SwitchState> makeSwitchState(
      const std::shared_ptr<SystemPortMap>& sysPorts,
      const std::shared_ptr<InterfaceMap>& intfs) const {
    auto state = std::make_shared<SwitchState>();
    auto mSysPorts = std::make_shared<MultiSwitchSystemPortMap>();
    auto mIntfs = std::make_shared<MultiSwitchInterfaceMap>();
    CHECK(!sysPorts->empty());
    HwSwitchMatcher matcher(
        std::unordered_set<SwitchID>{
            sysPorts->cbegin()->second->getSwitchId()});
    mSysPorts->addMapNode(sysPorts, matcher);
    mIntfs->addMapNode(intfs, matcher);
    state->resetSystemPorts(mSysPorts);
    state->resetIntfs(mIntfs);
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
  std::unique_ptr<DsfSubscription> createSubscription(
      const std::string& remoteEndpoint = "remote",
      const folly::IPAddress& remoteIp = folly::IPAddress("::1")) {
    fsdb::SubscriptionOptions opts{
        "test-sub", false /* subscribeStats */, FLAGS_dsf_gr_hold_time};
    return std::make_unique<DsfSubscription>(
        std::move(opts),
        streamConnectPool_->getEventBase(),
        streamServePool_->getEventBase(),
        hwUpdatePool_->getEventBase(),
        "local",
        remoteEndpoint,
        remoteSwitchIds(),
        folly::IPAddress("::1"),
        remoteIp,
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

  // Run multiple threads concurrently with synchronized start
  void runConcurrentThreads(std::vector<std::function<void()>>& threadFns) {
    std::atomic<bool> startFlag{false};
    std::vector<std::thread> threads;
    threads.reserve(threadFns.size());

    for (auto& fn : threadFns) {
      threads.emplace_back([&startFlag, fn]() {
        while (!startFlag.load()) {
          std::this_thread::yield();
        }
        fn();
      });
    }

    // Start all threads simultaneously
    startFlag.store(true);

    // Wait for all threads to complete
    for (auto& thread : threads) {
      thread.join();
    }
  }

  // Wait for event base queue to drain
  void waitForQueueDrain() {
    WITH_RETRIES({
      ASSERT_EVENTUALLY_EQ(
          this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(), 0);
    });
  }

  // Verify final state after updates
  void verifySysPortAndRif(
      size_t beforeNumSysPorts,
      size_t beforeNumRifs,
      int expectedSysPortsPerSwitch) {
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(
          this->getRemoteSystemPorts()->size(),
          beforeNumSysPorts +
              (this->kNumRemoteSwitchAsics * expectedSysPortsPerSwitch));
      EXPECT_EVENTUALLY_EQ(
          this->getRemoteInterfaces()->size(),
          beforeNumRifs +
              (this->kNumRemoteSwitchAsics * expectedSysPortsPerSwitch));
    });
  }

 protected:
  void verifyRemoteIntfRouteDelta(
      StateDelta delta,
      int expectedRouteAdded,
      int expectedRouteDeleted) {
    auto routesAdded = 0;
    auto routesDeleted = 0;

    for (const auto& fibsInfoDelta : delta.getFibsInfoDelta()) {
      for (const auto& routeDelta : fibsInfoDelta.getFibsMapDelta()) {
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
  std::unique_ptr<HwTestHandle> handle_;
  std::shared_ptr<DsfSubscription> subscription_;
  SwSwitch* sw_;
};

TYPED_TEST_SUITE(DsfSubscriptionTest, TestTypes);

TYPED_TEST(DsfSubscriptionTest, Connect) {
  this->createPublisher();
  auto state = this->makeSwitchState();
  this->publishSwitchState(state);
  std::optional<std::map<SwitchID, std::shared_ptr<SystemPortMap>>>
      recvSysPorts;
  std::optional<std::map<SwitchID, std::shared_ptr<InterfaceMap>>> recvIntfs;
  this->subscription_ = this->createSubscription();
  WITH_RETRIES({
    // 2 sys ports added per ASIC - 1 RCY, 1 Dynamic
    ASSERT_EVENTUALLY_EQ(
        this->getRemoteSystemPorts()->size(), this->kNumRemoteSwitchAsics * 2);
    ASSERT_EVENTUALLY_EQ(
        this->getRemoteInterfaces()->size(),
        this->getRemoteSystemPorts()->size());
    ASSERT_EVENTUALLY_EQ(
        this->dsfSessionState(), DsfSessionState::WAIT_FOR_REMOTE);
  });

  this->updateDsfSubscriberState(
      "local", fsdb::FsdbSubscriptionState::CONNECTED);
  WITH_RETRIES(ASSERT_EVENTUALLY_EQ(
      this->dsfSessionState(), DsfSessionState::ESTABLISHED));
}

TYPED_TEST(DsfSubscriptionTest, ConnectDisconnect) {
  this->createPublisher();
  auto state = this->makeSwitchState();
  this->publishSwitchState(state);
  std::optional<std::map<SwitchID, std::shared_ptr<SystemPortMap>>>
      recvSysPorts;
  std::optional<std::map<SwitchID, std::shared_ptr<InterfaceMap>>> recvIntfs;
  this->subscription_ = this->createSubscription();

  WITH_RETRIES({
    // 2 sys ports added per ASIC - 1 RCY, 1 Dynamic
    ASSERT_EVENTUALLY_EQ(
        this->getRemoteSystemPorts()->size(), this->kNumRemoteSwitchAsics * 2);
    ASSERT_EVENTUALLY_EQ(
        this->getRemoteInterfaces()->size(),
        this->getRemoteSystemPorts()->size());
    ASSERT_EVENTUALLY_EQ(
        this->dsfSessionState(), DsfSessionState::WAIT_FOR_REMOTE);
  });

  this->stopPublisher();
  WITH_RETRIES(
      ASSERT_EVENTUALLY_EQ(this->dsfSessionState(), DsfSessionState::CONNECT));
}

TYPED_TEST(DsfSubscriptionTest, GR) {
  this->createPublisher();
  auto state = this->makeSwitchState();
  this->publishSwitchState(state);
  FLAGS_dsf_gr_hold_time = 5;
  this->subscription_ = this->createSubscription();

  WITH_RETRIES({
    // 2 sys ports added per ASIC - 1 RCY, 1 Dynamic
    ASSERT_EVENTUALLY_EQ(
        this->getRemoteSystemPorts()->size(), this->kNumRemoteSwitchAsics * 2);
    ASSERT_EVENTUALLY_EQ(
        this->getRemoteInterfaces()->size(),
        this->getRemoteSystemPorts()->size());
    ASSERT_EVENTUALLY_EQ(
        this->dsfSessionState(), DsfSessionState::WAIT_FOR_REMOTE);
  });

  this->stopPublisher(true);
  this->createPublisher();
  this->publishSwitchState(state);

  WITH_RETRIES({
    // 2 sys ports added per ASIC - 1 RCY, 1 Dynamic
    ASSERT_EVENTUALLY_EQ(
        this->getRemoteSystemPorts()->size(), this->kNumRemoteSwitchAsics * 2);
    ASSERT_EVENTUALLY_EQ(
        this->getRemoteInterfaces()->size(),
        this->getRemoteSystemPorts()->size());
    ASSERT_EVENTUALLY_EQ(
        this->dsfSessionState(), DsfSessionState::WAIT_FOR_REMOTE);
    auto remoteRifs = this->getRemoteInterfaces();
    for (const auto [_, rif] : std::as_const(*remoteRifs)) {
      ASSERT_EVENTUALLY_EQ(rif->getNdpTable()->size(), 1);
      ASSERT_EVENTUALLY_EQ(rif->getArpTable()->size(), 1);
    }
  });
  auto assertStatus = [this](LivenessStatus expectedStatus) {
    auto assertObjStatus = [expectedStatus](const auto& objs) {
      std::for_each(
          objs->begin(), objs->end(), [expectedStatus](const auto& idAndObj) {
            if (!idAndObj.second->isStatic()) {
              EXPECT_EQ(
                  idAndObj.second->getRemoteLivenessStatus(), expectedStatus);
            }
          });
    };
    assertObjStatus(this->getRemoteSystemPorts());
    assertObjStatus(this->getRemoteInterfaces());
  };
  CounterCache counters(this->sw_);
  // Should be LIVE before GR expire
  assertStatus(LivenessStatus::LIVE);
  this->stopPublisher(true);
  auto grExpiredCounter =
      SwitchStats::kCounterPrefix + "dsfsession_gr_expired.sum.60";
  WITH_RETRIES({
    counters.update();
    ASSERT_EVENTUALLY_EQ(this->dsfSessionState(), DsfSessionState::CONNECT);
    ASSERT_EVENTUALLY_TRUE(counters.checkExist(grExpiredCounter));
    ASSERT_EVENTUALLY_EQ(counters.value(grExpiredCounter), 1);

    auto remoteRifs = this->getRemoteInterfaces();
    for (const auto [_, rif] : std::as_const(*remoteRifs)) {
      // Neighbors should get pruned
      if (!rif->isStatic()) {
        ASSERT_EVENTUALLY_EQ(rif->getNdpTable()->size(), 0);
        ASSERT_EVENTUALLY_EQ(rif->getArpTable()->size(), 0);
      }
    }
  });
  // Should be STATLE after GR expire
  assertStatus(LivenessStatus::STALE);
}

TYPED_TEST(DsfSubscriptionTest, DataUpdate) {
  this->createPublisher();
  auto state = this->makeSwitchState();
  this->publishSwitchState(state);

  std::optional<std::map<SwitchID, std::shared_ptr<SystemPortMap>>>
      recvSysPorts;
  std::optional<std::map<SwitchID, std::shared_ptr<InterfaceMap>>> recvIntfs;
  this->subscription_ = this->createSubscription();

  WITH_RETRIES({
    // 2 sys ports added per ASIC - 1 RCY, 1 Dynamic
    ASSERT_EVENTUALLY_EQ(
        this->getRemoteSystemPorts()->size(), this->kNumRemoteSwitchAsics * 2);
    ASSERT_EVENTUALLY_EQ(
        this->getRemoteInterfaces()->size(),
        this->getRemoteSystemPorts()->size());
    ASSERT_EVENTUALLY_EQ(
        this->dsfSessionState(), DsfSessionState::WAIT_FOR_REMOTE);
  });

  auto sysPort2 = makeSysPort(
      std::nullopt,
      SystemPortID(kSysPortRangeMin + kDynamicSysPortsOffset + 1),
      kRemoteSwitchIdBegin);
  auto portMap = state->getSystemPorts()->modify(&state);
  portMap->addNode(sysPort2, this->matcher());
  this->publishSwitchState(state);

  WITH_RETRIES(ASSERT_EVENTUALLY_EQ(
      this->getRemoteSystemPorts()->size(),
      (this->kNumRemoteSwitchAsics * 2) + 1));
}

TYPED_TEST(DsfSubscriptionTest, updateFailed) {
  CounterCache counters(this->sw_);
  this->createPublisher();
  auto state = this->makeSwitchState();
  this->publishSwitchState(state);

  std::optional<std::map<SwitchID, std::shared_ptr<SystemPortMap>>>
      recvSysPorts;
  std::optional<std::map<SwitchID, std::shared_ptr<InterfaceMap>>> recvIntfs;
  this->subscription_ = this->createSubscription();

  WITH_RETRIES({
    // 2 sys ports added per ASIC - 1 RCY, 1 Dynamic
    ASSERT_EVENTUALLY_EQ(
        this->getRemoteSystemPorts()->size(), this->kNumRemoteSwitchAsics * 2);
    ASSERT_EVENTUALLY_EQ(
        this->getRemoteInterfaces()->size(),
        this->getRemoteSystemPorts()->size());
    ASSERT_EVENTUALLY_EQ(
        this->dsfSessionState(), DsfSessionState::WAIT_FOR_REMOTE);
  });
  waitForStateUpdates(this->sw_);

  // Fail HW update by returning current state
  EXPECT_HW_CALL(this->sw_, stateChangedImpl(_))
      .Times(::testing::AtLeast(1))
      .WillOnce(Return(this->sw_->getState()));
  auto sysPort2 = makeSysPort(
      std::nullopt,
      SystemPortID(kSysPortRangeMin + kDynamicSysPortsOffset + 1),
      kRemoteSwitchIdBegin);
  auto portMap = state->getSystemPorts()->modify(&state);
  portMap->addNode(sysPort2, this->matcher());
  this->publishSwitchState(state);
  auto dsfUpdateFailedCounter =
      SwitchStats::kCounterPrefix + "dsf_update_failed.sum.60";
  WITH_RETRIES({
    counters.update();
    ASSERT_EVENTUALLY_TRUE(counters.checkExist(dsfUpdateFailedCounter));
    ASSERT_EVENTUALLY_EQ(counters.value(dsfUpdateFailedCounter), 1);
  });
}

TYPED_TEST(DsfSubscriptionTest, updateWithRollbackProtection) {
  auto sysPorts = makeSysPortsForSwitchIds(
      std::set<SwitchID>({SwitchID(kRemoteSwitchIdBegin)}), 2);
  auto rifs = makeRifs(sysPorts.get());

  std::map<SwitchID, std::shared_ptr<SystemPortMap>> switchId2SystemPorts;
  std::map<SwitchID, std::shared_ptr<InterfaceMap>> switchId2Intfs;
  switchId2SystemPorts[SwitchID(kRemoteSwitchIdBegin)] = sysPorts;
  switchId2Intfs[SwitchID(kRemoteSwitchIdBegin)] = rifs;

  // Add remote interfaces
  const auto prevState = this->sw_->getState();
  this->subscription_ = this->createSubscription();
  this->subscription_->updateWithRollbackProtection(
      switchId2SystemPorts, switchId2Intfs);

  const auto addedState = this->sw_->getState();
  this->verifyRemoteIntfRouteDelta(StateDelta(prevState, addedState), 2, 0);

  // Reapply config - ensure no change in remote interfaces
  this->sw_->applyConfig("Reload initial config", this->initialConfig());
  const auto configReappliedState = this->sw_->getState();
  this->verifyRemoteIntfRouteDelta(
      StateDelta(addedState, configReappliedState), 0, 0);

  // Change remote interface routes
  switchId2SystemPorts[SwitchID(kRemoteSwitchIdBegin)] =
      makeSysPortsForSwitchIds(
          std::set<SwitchID>({SwitchID(kRemoteSwitchIdBegin)}), 2);
  switchId2Intfs[SwitchID(kRemoteSwitchIdBegin)] = makeRifs(sysPorts.get());

  const auto sysPort1Id = kSysPortRangeMin + kDynamicSysPortsOffset;
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

  this->subscription_->updateWithRollbackProtection(
      switchId2SystemPorts, switchId2Intfs);

  auto modifiedState = this->sw_->getState();
  this->verifyRemoteIntfRouteDelta(
      StateDelta(configReappliedState, modifiedState), 2, 2);

  // Remove remote interface routes
  switchId2SystemPorts[SwitchID(kRemoteSwitchIdBegin)] =
      std::make_shared<SystemPortMap>();
  switchId2Intfs[SwitchID(kRemoteSwitchIdBegin)] =
      std::make_shared<InterfaceMap>();

  this->subscription_->updateWithRollbackProtection(
      switchId2SystemPorts, switchId2Intfs);

  waitForStateUpdates(this->sw_);
  auto deletedState = this->sw_->getState();
  this->verifyRemoteIntfRouteDelta(
      StateDelta(modifiedState, deletedState), 0, 2);
}

TYPED_TEST(DsfSubscriptionTest, setupNeighbors) {
  this->subscription_ = this->createSubscription();
  auto updateAndCompareTables = [this](
                                    const auto& sysPorts,
                                    const auto& rifs,
                                    bool publishState,
                                    bool noNeighbors = false) {
    if (publishState) {
      rifs->publish();
    }

    // this->subscription_->updateWithRollbackProtection is expected to set
    // isLocal to False, and rest of the structure should remain the same.
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

    this->subscription_->updateWithRollbackProtection(
        switchId2SystemPorts, switchId2Intfs);

    waitForStateUpdates(this->sw_);

    for (const auto& [_, intfMap] :
         std::as_const(*this->sw_->getState()->getRemoteInterfaces())) {
      for (const auto& [_, localRif] : std::as_const(*intfMap)) {
        if (localRif->isStatic()) {
          continue;
        }
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
    // neighbor entries are modified to set isLocal=false
    // Thus, if neighbor table is non-empty, programmed vs. actually
    // programmed would be unequal for published state.
    // for unpublished state, the passed state would be modified, and thus,
    // programmed vs actually programmed state would be equal.
    EXPECT_TRUE(
        rifs->toThrift() !=
            this->sw_->getState()
                ->getRemoteInterfaces()
                ->getAllNodes()
                ->toThrift() ||
        noNeighbors || !publishState);
  };

  auto verifySetupNeighbors = [&](bool publishState) {
    {
      // No neighbors
      auto sysPorts = makeSysPortsForSwitchIds(
          std::set<SwitchID>({SwitchID(kRemoteSwitchIdBegin)}));
      auto rifs = makeRifs(sysPorts.get());
      updateAndCompareTables(
          sysPorts, rifs, publishState, true /* noNeighbors */);
    }
    {
      // add neighbors
      auto sysPorts = makeSysPortsForSwitchIds(
          std::set<SwitchID>({SwitchID(kRemoteSwitchIdBegin)}));
      auto rifs = makeRifs(sysPorts.get());
      auto firstRif = kSysPortRangeMin + kDynamicSysPortsOffset;
      auto [ndpTable, arpTable] = makeNbrs();
      rifs->ref(firstRif)->setNdpTable(ndpTable);
      rifs->ref(firstRif)->setArpTable(arpTable);
      updateAndCompareTables(sysPorts, rifs, publishState);
    }
    {
      // update neighbors
      auto sysPorts = makeSysPortsForSwitchIds(
          std::set<SwitchID>({SwitchID(kRemoteSwitchIdBegin)}));
      auto rifs = makeRifs(sysPorts.get());
      auto firstRif = kSysPortRangeMin + kDynamicSysPortsOffset;
      auto [ndpTable, arpTable] = makeNbrs();
      ndpTable.begin()->second.mac() = "06:05:04:03:02:01";
      arpTable.begin()->second.mac() = "06:05:04:03:02:01";
      rifs->ref(firstRif)->setNdpTable(ndpTable);
      rifs->ref(firstRif)->setArpTable(arpTable);
      updateAndCompareTables(sysPorts, rifs, publishState);
    }
    {
      // delete neighbors
      auto sysPorts = makeSysPortsForSwitchIds(
          std::set<SwitchID>({SwitchID(kRemoteSwitchIdBegin)}));
      auto rifs = makeRifs(sysPorts.get());
      auto firstRif = kSysPortRangeMin + kDynamicSysPortsOffset;
      auto [ndpTable, arpTable] = makeNbrs();
      ndpTable.erase(ndpTable.begin());
      arpTable.erase(arpTable.begin());
      rifs->ref(firstRif)->setNdpTable(ndpTable);
      rifs->ref(firstRif)->setArpTable(arpTable);
      updateAndCompareTables(sysPorts, rifs, publishState);
    }
    {
      // clear neighbors
      auto sysPorts = makeSysPortsForSwitchIds(
          std::set<SwitchID>({SwitchID(kRemoteSwitchIdBegin)}));
      auto rifs = makeRifs(sysPorts.get());
      updateAndCompareTables(
          sysPorts, rifs, publishState, true /* noNeighbors */);
    }
  };

  verifySetupNeighbors(false /* publishState */);
  verifySetupNeighbors(true /* publishState */);
}

TYPED_TEST(DsfSubscriptionTest, DataUpdateForLocalSwitchId) {
  CounterCache counters(this->sw_);
  this->createPublisher();
  auto state = this->makeSwitchState();
  this->publishSwitchState(state);
  this->subscription_ = this->createSubscription();
  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(
        this->dsfSessionState(), DsfSessionState::WAIT_FOR_REMOTE);
  });
  auto localSwitchId = *this->sw_->getSwitchInfoTable().getSwitchIDs().begin();
  auto sysPorts = makeSysPortsForSwitchIds(std::set<SwitchID>({localSwitchId}));
  auto rifs = makeRifs(sysPorts.get());
  state = this->makeSwitchState(sysPorts, rifs);
  this->publishSwitchState(state);
  auto dsfUpdateFailedCounter =
      SwitchStats::kCounterPrefix + "dsf_update_failed.sum.60";
  WITH_RETRIES({
    counters.update();
    ASSERT_EVENTUALLY_TRUE(counters.checkExist(dsfUpdateFailedCounter));
    ASSERT_EVENTUALLY_GE(counters.value(dsfUpdateFailedCounter), 1);
  });
}

TYPED_TEST(DsfSubscriptionTest, FirstUpdateFailsValidation) {
  CounterCache counters(this->sw_);
  auto localSwitchId = *this->sw_->getSwitchInfoTable().getSwitchIDs().begin();
  auto sysPorts = makeSysPortsForSwitchIds(std::set<SwitchID>({localSwitchId}));
  auto rifs = makeRifs(sysPorts.get());
  auto state = this->makeSwitchState(sysPorts, rifs);
  this->createPublisher();
  this->publishSwitchState(state);
  this->subscription_ = this->createSubscription();
  auto dsfUpdateFailedCounter =
      SwitchStats::kCounterPrefix + "dsf_update_failed.sum.60";
  WITH_RETRIES({
    counters.update();
    ASSERT_EVENTUALLY_TRUE(counters.checkExist(dsfUpdateFailedCounter));
    ASSERT_EVENTUALLY_GE(counters.value(dsfUpdateFailedCounter), 1);
  });
  // Session should never get established now, since connection establish
  // should itself fail
  EXPECT_NE(this->dsfSessionState(), DsfSessionState::WAIT_FOR_REMOTE);
  EXPECT_NE(this->dsfSessionState(), DsfSessionState::REMOTE_DISCONNECTED);
  EXPECT_NE(this->dsfSessionState(), DsfSessionState::ESTABLISHED);
}

TYPED_TEST(DsfSubscriptionTest, BogusIntfAdd) {
  CounterCache counters(this->sw_);
  auto localSwitchId = *this->sw_->getSwitchInfoTable().getSwitchIDs().begin();
  auto sysPorts = this->makeSysPorts();
  auto rifs = makeRifs(sysPorts.get());
  auto state = this->makeSwitchState(sysPorts, rifs);
  this->createPublisher();
  this->publishSwitchState(state);
  this->subscription_ = this->createSubscription();
  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(
        this->dsfSessionState(), DsfSessionState::WAIT_FOR_REMOTE);
  });
  auto newSysPorts = this->makeSysPorts(2);
  auto newRifs = makeRifs(newSysPorts.get());
  auto newState = this->makeSwitchState(sysPorts, newRifs);
  this->publishSwitchState(newState);
  auto dsfUpdateFailedCounter =
      SwitchStats::kCounterPrefix + "dsf_update_failed.sum.60";
  WITH_RETRIES({
    counters.update();
    ASSERT_EVENTUALLY_TRUE(counters.checkExist(dsfUpdateFailedCounter));
    ASSERT_EVENTUALLY_GE(counters.value(dsfUpdateFailedCounter), 1);
  });
}

TYPED_TEST(DsfSubscriptionTest, RemoteEndpointString) {
  this->createPublisher();
  auto state = this->makeSwitchState();
  this->publishSwitchState(state);
  std::optional<std::map<SwitchID, std::shared_ptr<SystemPortMap>>>
      recvSysPorts;
  std::optional<std::map<SwitchID, std::shared_ptr<InterfaceMap>>> recvIntfs;
  this->subscription_ = this->createSubscription();
  std::unique_ptr<DsfSubscription> subscription2 =
      this->createSubscription("remote2", folly::IPAddress("::2"));

  std::string expectedEndpointStr = "remote_::1";
  EXPECT_EQ(this->subscription_->remoteEndpointStr(), expectedEndpointStr);
  std::string expectedEndpoint2Str = "remote2_::2";
  EXPECT_EQ(subscription2->remoteEndpointStr(), expectedEndpoint2Str);
}

TYPED_TEST(DsfSubscriptionTest, QueueDsfUpdateRaceCondition) {
  // Test to reproduce race condition in queueDsfUpdate where:
  // 1. queueDsfUpdate is called - one update queued to hwUpdateEvb_
  // 2. processGRHoldTimerExpired is invoked - queues another event
  // 3. queueDsfUpdate is called again - (prior to the fix) if update from 1 not
  // yet processed, it will update nextDsfUpdate_ instead of queuing to
  // hwUpdateEvb_ The effect is only 2 events in the queue instead of 3, and the
  // last event clears all entries, so update from step 3 is lost.

  this->subscription_ = this->createSubscription();

  // Use Baton to block hwUpdateEvb_ and simulate the race condition
  folly::Baton<> baton;
  this->hwUpdatePool_->getEventBase()->runInEventBaseThread(
      [&]() { baton.wait(); });
  // Wait for the blocking event to be in progress
  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(
        this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(), 0);
  });

  auto initialQueueSize =
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize();

  auto queueSysPortUpdate = [&](int numSysPorts) {
    DsfSubscription::DsfUpdate update;
    for (const auto& remoteSwitchId : this->remoteSwitchIds()) {
      auto sysPorts = makeSysPortsForSwitchIds({remoteSwitchId}, numSysPorts);
      auto rifs = makeRifs(sysPorts.get());
      update.switchId2SystemPorts[remoteSwitchId] = sysPorts;
      update.switchId2Intfs[remoteSwitchId] = rifs;
    }
    this->subscription_->queueDsfUpdate(std::move(update));
  };

  auto beforeNumSysPorts = this->getRemoteSystemPorts()->size();
  auto beforeNumRifs = this->getRemoteInterfaces()->size();
  auto finalNumSysPorts = 2;

  // Step 1: First queueDsfUpdate call - should queue one event
  queueSysPortUpdate(1 /* numSysPorts */);

  // Verify one event was queued
  auto queueSizeAfterFirst =
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize();
  EXPECT_EQ(queueSizeAfterFirst, initialQueueSize + 1);

  // Step 2: Call processGRHoldTimerExpired - should queue another event
  this->subscription_->processGRHoldTimerExpired();

  // Verify second event was queued
  auto queueSizeAfterGR =
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize();
  EXPECT_EQ(queueSizeAfterGR, initialQueueSize + 2);

  // Step 3: Second queueDsfUpdate call - should queue a third event.
  queueSysPortUpdate(finalNumSysPorts);

  // Verify the fix of race condition: there should be only 3 events in the
  // queue. The GR expiry should not be the last event.
  auto finalQueueSize =
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize();
  EXPECT_EQ(finalQueueSize, initialQueueSize + 3);

  // Unblock the event base to process all queued events
  baton.post();

  // Wait for all events to be processed and exit
  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(
        this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(), 0);
  });

  WITH_RETRIES({
    auto remoteRifs = this->getRemoteInterfaces();
    EXPECT_EVENTUALLY_EQ(
        remoteRifs->size(),
        beforeNumRifs + (this->kNumRemoteSwitchAsics * finalNumSysPorts));
    EXPECT_EVENTUALLY_EQ(
        this->getRemoteSystemPorts()->size(),
        beforeNumSysPorts + (this->kNumRemoteSwitchAsics * finalNumSysPorts));
    for (const auto [_, rif] : std::as_const(*remoteRifs)) {
      EXPECT_EVENTUALLY_EQ(rif->getNdpTable()->size(), 1);
      EXPECT_EVENTUALLY_EQ(rif->getArpTable()->size(), 1);
    }
  });
}

TYPED_TEST(DsfSubscriptionTest, MultipleQueuedDsfUpdatesCoalesce) {
  // Test that multiple DSF updates queued in quick succession are coalesced
  // when no GR event occurs. Only the latest update should be processed.
  this->subscription_ = this->createSubscription();

  // Use Baton to block hwUpdateEvb_
  folly::Baton<> baton;
  this->hwUpdatePool_->getEventBase()->runInEventBaseThread(
      [&]() { baton.wait(); });

  // Wait for the blocking event to be in progress
  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(
        this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(), 0);
  });

  auto initialQueueSize =
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize();
  auto beforeNumSysPorts = this->getRemoteSystemPorts()->size();
  auto beforeNumRifs = this->getRemoteInterfaces()->size();

  auto queueSysPortUpdate = [&](int numSysPorts) {
    DsfSubscription::DsfUpdate update;
    for (const auto& remoteSwitchId : this->remoteSwitchIds()) {
      auto sysPorts = makeSysPortsForSwitchIds({remoteSwitchId}, numSysPorts);
      auto rifs = makeRifs(sysPorts.get());
      update.switchId2SystemPorts[remoteSwitchId] = sysPorts;
      update.switchId2Intfs[remoteSwitchId] = rifs;
    }
    this->subscription_->queueDsfUpdate(std::move(update));
  };

  // Queue first update with 1 sysport per switch
  queueSysPortUpdate(1 /*numSysPorts*/);

  // First update should add one event
  EXPECT_EQ(
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(),
      initialQueueSize + 1);

  // Queue second update with 2 sysports per switch - should coalesce
  queueSysPortUpdate(2 /*numSysPorts*/);

  // Second update should NOT add a new event (coalesced with first)
  EXPECT_EQ(
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(),
      initialQueueSize + 1);

  // Queue third update with 3 sysports per switch - should also coalesce
  queueSysPortUpdate(3 /*numSysPorts*/);

  // Third update should still be coalesced - only 1 event total
  EXPECT_EQ(
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(),
      initialQueueSize + 1);

  // Unblock the event base to process all queued events
  baton.post();

  // Wait for all events to be processed
  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(
        this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(), 0);
  });

  // Verify the final state reflects the LAST update (3 sysports per switch)
  WITH_RETRIES({
    EXPECT_EVENTUALLY_EQ(
        this->getRemoteSystemPorts()->size(),
        beforeNumSysPorts + (this->kNumRemoteSwitchAsics * 3));
    EXPECT_EVENTUALLY_EQ(
        this->getRemoteInterfaces()->size(),
        beforeNumRifs + (this->kNumRemoteSwitchAsics * 3));
  });
}

TYPED_TEST(DsfSubscriptionTest, GREventSeparatesUpdates) {
  // Test that when a GR event occurs between DSF updates, both updates
  // are processed separately (not coalesced).
  this->subscription_ = this->createSubscription();

  // Use Baton to block hwUpdateEvb_
  folly::Baton<> baton;
  this->hwUpdatePool_->getEventBase()->runInEventBaseThread(
      [&]() { baton.wait(); });

  // Wait for the blocking event to be in progress
  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(
        this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(), 0);
  });

  auto initialQueueSize =
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize();
  auto beforeNumSysPorts = this->getRemoteSystemPorts()->size();
  auto beforeNumRifs = this->getRemoteInterfaces()->size();

  auto queueSysPortUpdate = [&](int numSysPorts) {
    DsfSubscription::DsfUpdate update;
    for (const auto& remoteSwitchId : this->remoteSwitchIds()) {
      auto sysPorts = makeSysPortsForSwitchIds({remoteSwitchId}, numSysPorts);
      auto rifs = makeRifs(sysPorts.get());
      update.switchId2SystemPorts[remoteSwitchId] = sysPorts;
      update.switchId2Intfs[remoteSwitchId] = rifs;
    }
    this->subscription_->queueDsfUpdate(std::move(update));
  };

  // Queue first update
  queueSysPortUpdate(1 /*numSysPorts*/);

  EXPECT_EQ(
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(),
      initialQueueSize + 1);

  // Trigger GR event - this should set lastEventGR_ flag
  this->subscription_->processGRHoldTimerExpired();

  EXPECT_EQ(
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(),
      initialQueueSize + 2);

  // Queue second update - should NOT coalesce due to GR event
  queueSysPortUpdate(2 /*numSysPorts*/);

  // Should have 3 events: first update, GR, second update
  EXPECT_EQ(
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(),
      initialQueueSize + 3);

  // Queue third update - should coalesce with second (no GR between them)
  queueSysPortUpdate(3 /*numSysPorts*/);

  // Still 3 events (third coalesced with second)
  EXPECT_EQ(
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(),
      initialQueueSize + 3);

  // Unblock the event base to process all queued events
  baton.post();

  // Wait for all events to be processed
  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(
        this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(), 0);
  });

  // Verify the final state reflects the last update (3 sysports per switch)
  WITH_RETRIES({
    EXPECT_EVENTUALLY_EQ(
        this->getRemoteSystemPorts()->size(),
        beforeNumSysPorts + (this->kNumRemoteSwitchAsics * 3));
    EXPECT_EVENTUALLY_EQ(
        this->getRemoteInterfaces()->size(),
        beforeNumRifs + (this->kNumRemoteSwitchAsics * 3));
  });
}

TYPED_TEST(DsfSubscriptionTest, MultipleGREventsSeparateUpdates) {
  // Test that multiple GR events each cause subsequent updates to be
  // queued separately.
  this->subscription_ = this->createSubscription();

  // Use Baton to block hwUpdateEvb_
  folly::Baton<> baton;
  this->hwUpdatePool_->getEventBase()->runInEventBaseThread(
      [&]() { baton.wait(); });

  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(
        this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(), 0);
  });

  auto initialQueueSize =
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize();
  auto beforeNumSysPorts = this->getRemoteSystemPorts()->size();
  auto beforeNumRifs = this->getRemoteInterfaces()->size();

  auto queueSysPortUpdate = [&](int numSysPorts) {
    DsfSubscription::DsfUpdate update;
    for (const auto& remoteSwitchId : this->remoteSwitchIds()) {
      auto sysPorts = makeSysPortsForSwitchIds({remoteSwitchId}, numSysPorts);
      auto rifs = makeRifs(sysPorts.get());
      update.switchId2SystemPorts[remoteSwitchId] = sysPorts;
      update.switchId2Intfs[remoteSwitchId] = rifs;
    }
    this->subscription_->queueDsfUpdate(std::move(update));
  };

  // Sequence: Update1 -> GR1 -> Update2 -> GR2 -> Update3
  // Expected events: 5 total

  // Update 1
  queueSysPortUpdate(1 /*numSysPorts*/);
  EXPECT_EQ(
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(),
      initialQueueSize + 1);

  // GR 1
  this->subscription_->processGRHoldTimerExpired();
  EXPECT_EQ(
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(),
      initialQueueSize + 2);

  // Update 2 (should not coalesce due to GR1)
  queueSysPortUpdate(2 /*numSysPorts*/);
  EXPECT_EQ(
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(),
      initialQueueSize + 3);

  // GR 2
  this->subscription_->processGRHoldTimerExpired();
  EXPECT_EQ(
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(),
      initialQueueSize + 4);

  // Update 3 (should not coalesce due to GR2)
  queueSysPortUpdate(3 /*numSysPorts*/);
  EXPECT_EQ(
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(),
      initialQueueSize + 5);

  // Unblock the event base to process all queued events
  baton.post();

  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(
        this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(), 0);
  });

  // Final state should reflect last update
  WITH_RETRIES({
    EXPECT_EVENTUALLY_EQ(
        this->getRemoteSystemPorts()->size(),
        beforeNumSysPorts + (this->kNumRemoteSwitchAsics * 3));
    EXPECT_EVENTUALLY_EQ(
        this->getRemoteInterfaces()->size(),
        beforeNumRifs + (this->kNumRemoteSwitchAsics * 3));
  });
}

TYPED_TEST(DsfSubscriptionTest, UpdateSkippedWhenNewerUpdatesQueued) {
  // Test that an update is skipped when newer updates exist in the queue
  // after a GR event. This verifies the optimization in queueDsfUpdate where
  // needsUpdate = dsfUpdateQueue_.empty() - if the queue is not empty after
  // dequeueing the current update, the current update is skipped.
  //
  // Scenario: Update1 -> GR -> Update2
  // When Update1's event runs, it dequeues Update1 but sees Update2 still in
  // the queue, so it skips processing (needsUpdate = false).
  //
  // We verify this by checking the SW switch state generation number:
  // - If Update1 is skipped: 2 state updates (GR + Update2)
  // - If Update1 is NOT skipped: 3 state updates (Update1 + GR + Update2)
  this->subscription_ = this->createSubscription();

  // Use Baton to block hwUpdateEvb_ and simulate the race condition
  folly::Baton<> baton;
  this->hwUpdatePool_->getEventBase()->runInEventBaseThread(
      [&]() { baton.wait(); });
  // Wait for the blocking event to be in progress
  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(
        this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(), 0);
  });

  auto initialQueueSize =
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize();

  // Record the initial state generation
  auto initialStateGeneration = this->sw_->getState()->getGeneration();

  auto queueSysPortUpdate = [&](int numSysPorts) {
    DsfSubscription::DsfUpdate update;
    for (const auto& remoteSwitchId : this->remoteSwitchIds()) {
      auto sysPorts = makeSysPortsForSwitchIds({remoteSwitchId}, numSysPorts);
      auto rifs = makeRifs(sysPorts.get());
      update.switchId2SystemPorts[remoteSwitchId] = sysPorts;
      update.switchId2Intfs[remoteSwitchId] = rifs;
    }
    this->subscription_->queueDsfUpdate(std::move(update));
  };

  // Queue Update1 with 10 sysports per switch
  queueSysPortUpdate(10);
  EXPECT_EQ(
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(),
      initialQueueSize + 1);

  // Trigger GR event - this sets lastEventGR_ flag
  this->subscription_->processGRHoldTimerExpired();
  EXPECT_EQ(
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(),
      initialQueueSize + 2);

  // Queue Update2 with 2 sysports per switch - because of GR, this will
  // be queued as a separate entry. When Update1's event runs, it will see
  // Update2 in the queue and skip processing Update1.
  queueSysPortUpdate(2);
  EXPECT_EQ(
      this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(),
      initialQueueSize + 3);

  // Unblock the event base to process all queued events
  baton.post();

  // Wait for all events to be processed
  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(
        this->hwUpdatePool_->getEventBase()->getNotificationQueueSize(), 0);
  });

  // Wait for state updates to complete
  waitForStateUpdates(this->sw_);

  // Verify the state generation only incremented by 2 (GR + Update2).
  // If Update1 was NOT skipped, the generation would increment by 3.
  // This proves Update1 was skipped because the queue was not empty after
  // dequeueing it (Update2 was still in the queue).
  auto finalStateGeneration = this->sw_->getState()->getGeneration();
  auto stateUpdates = finalStateGeneration - initialStateGeneration;

  // We expect exactly 2 state updates:
  // 1. GR event (processGRHoldTimerExpired calls updateDsfState)
  // 2. Update2 event (queue was empty after dequeue, so needsUpdate = true)
  // Update1 is skipped because queue was NOT empty after dequeue.
  EXPECT_EQ(stateUpdates, 2);
}

TYPED_TEST(DsfSubscriptionTest, ConcurrentQueueDsfUpdates) {
  // Test that multiple threads concurrently calling queueDsfUpdate() works
  // correctly. All updates should eventually be processed, and both the
  // dsfUpdateQueue and event base queue should be empty afterwards.
  this->subscription_ = this->createSubscription();

  auto beforeNumSysPorts = this->getRemoteSystemPorts()->size();
  auto beforeNumRifs = this->getRemoteInterfaces()->size();

  constexpr int kNumThreads = 10;
  constexpr int kUpdatesPerThread = 5;
  constexpr int kFinalNumSysPorts = 3;

  auto queueSysPortUpdate = [&](int numSysPorts) {
    DsfSubscription::DsfUpdate update;
    for (const auto& remoteSwitchId : this->remoteSwitchIds()) {
      auto sysPorts = makeSysPortsForSwitchIds({remoteSwitchId}, numSysPorts);
      auto rifs = makeRifs(sysPorts.get());
      update.switchId2SystemPorts[remoteSwitchId] = sysPorts;
      update.switchId2Intfs[remoteSwitchId] = rifs;
    }
    this->subscription_->queueDsfUpdate(std::move(update));
  };

  // Create thread functions
  std::vector<std::function<void()>> threadFns;
  threadFns.reserve(kNumThreads);
  for (int t = 0; t < kNumThreads; ++t) {
    threadFns.emplace_back([&, t]() {
      for (int i = 0; i < kUpdatesPerThread; ++i) {
        queueSysPortUpdate(
            (t * kUpdatesPerThread + i) % kUpdatesPerThread +
            1 /*numSysPorts*/);
      }
    });
  }

  // Run all threads concurrently
  this->runConcurrentThreads(threadFns);

  // Wait for all events to be processed
  this->waitForQueueDrain();

  // Verify dsfUpdateQueue_ is empty by checking we can still queue new updates
  // and they get processed normally
  queueSysPortUpdate(kFinalNumSysPorts /*numSysPorts*/);

  this->waitForQueueDrain();

  // Verify final state
  this->verifySysPortAndRif(
      beforeNumSysPorts, beforeNumRifs, kFinalNumSysPorts);
}

TYPED_TEST(DsfSubscriptionTest, ConcurrentQueueDsfUpdateAndGRExpiry) {
  // Test that multiple threads concurrently calling queueDsfUpdate and
  // processGRHoldTimerExpired do not cause deadlock. Both dsfUpdateQueue and
  // event base queue should be processed afterwards, and enqueueing new updates
  // should lead to normal processing.
  this->subscription_ = this->createSubscription();

  auto beforeNumSysPorts = this->getRemoteSystemPorts()->size();
  auto beforeNumRifs = this->getRemoteInterfaces()->size();

  constexpr int kNumUpdateThreads = 5;
  constexpr int kNumGRThreads = 3;
  constexpr int kUpdatesPerThread = 10;
  constexpr int kGRsPerThread = 5;
  constexpr int kFinalNumSysPorts = 4;

  auto queueSysPortUpdate = [&](int numSysPorts) {
    DsfSubscription::DsfUpdate update;
    for (const auto& remoteSwitchId : this->remoteSwitchIds()) {
      auto sysPorts = makeSysPortsForSwitchIds({remoteSwitchId}, numSysPorts);
      auto rifs = makeRifs(sysPorts.get());
      update.switchId2SystemPorts[remoteSwitchId] = sysPorts;
      update.switchId2Intfs[remoteSwitchId] = rifs;
    }
    this->subscription_->queueDsfUpdate(std::move(update));
  };

  // Create thread functions for both update and GR threads
  std::vector<std::function<void()>> threadFns;

  // Threads that call queueDsfUpdate
  threadFns.reserve(kNumUpdateThreads + kNumGRThreads);
  for (int t = 0; t < kNumUpdateThreads; ++t) {
    threadFns.emplace_back([&, t]() {
      for (int i = 0; i < kUpdatesPerThread; ++i) {
        queueSysPortUpdate(
            (t * kUpdatesPerThread + i) % kNumUpdateThreads +
            1 /*numSysPorts*/);
      }
    });
  }

  // Threads that call processGRHoldTimerExpired
  for (int t = 0; t < kNumGRThreads; ++t) {
    threadFns.emplace_back([&]() {
      for (int i = 0; i < kGRsPerThread; ++i) {
        this->subscription_->processGRHoldTimerExpired();
      }
    });
  }

  // Run all threads concurrently
  this->runConcurrentThreads(threadFns);

  // Wait for all events to be processed - should not deadlock
  this->waitForQueueDrain();

  // Verify the system is in a healthy state by enqueueing a new update
  // and checking it gets processed normally
  queueSysPortUpdate(kFinalNumSysPorts /*numSysPorts*/);

  this->waitForQueueDrain();

  // Verify final state
  this->verifySysPortAndRif(
      beforeNumSysPorts, beforeNumRifs, kFinalNumSysPorts);
}

} // namespace facebook::fboss
