// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "common/stats/MonotonicCounter.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/TeFlowEntry.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"
#include "folly/IPAddressV6.h"

using namespace facebook::fboss;

using facebook::network::toBinaryAddress;
using facebook::network::thrift::BinaryAddress;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::StringPiece;

using StateUpdateFn = facebook::fboss::SwSwitch::StateUpdateFn;

namespace {
static TeCounterID kCounterID("counter0");
static std::string kNhopAddrA("2401:db00:2110:3001::0002");
static std::string kNhopAddrB("2401:db00:2110:3055::0002");
static folly::MacAddress kMacAddress("01:02:03:04:05:06");
static VlanID kVlanA(1);
static VlanID kVlanB(55);
static PortID kPortIDA(1);
static PortID kPortIDB(11);
} // namespace

class TeFlowTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto config = testConfigA();
    cfg::ExactMatchTableConfig tableConfig;
    tableConfig.name() = "TeFlowTable";
    tableConfig.dstPrefixLength() = 64;
    config.switchSettings()->exactMatchTableConfigs() = {tableConfig};
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
    sw_->getNeighborUpdater()->receivedNdpMine(
        kVlanA,
        folly::IPAddressV6(kNhopAddrA),
        kMacAddress,
        PortDescriptor(kPortIDA),
        ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
        0);
    sw_->getNeighborUpdater()->receivedNdpMine(
        kVlanB,
        folly::IPAddressV6(kNhopAddrB),
        kMacAddress,
        PortDescriptor(kPortIDB),
        ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
        0);

    sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForBackgroundThread(sw_);
    waitForStateUpdates(sw_);
  }

 protected:
  IpPrefix ipPrefix(StringPiece ip, int length) {
    IpPrefix result;
    result.ip() = toBinaryAddress(IPAddress(ip));
    result.prefixLength() = length;
    return result;
  }

  TeFlow makeFlowKey(std::string dstIp, int prefixLength = 64) {
    TeFlow flow;
    flow.srcPort() = 100;
    flow.dstPrefix() = ipPrefix(dstIp, prefixLength);
    return flow;
  }

  std::shared_ptr<TeFlowEntry> makeFlowEntry(std::string dstIp) {
    auto flow = makeFlowKey(dstIp);
    auto flowEntry = std::make_shared<TeFlowEntry>(flow);
    std::vector<NextHopThrift> nexthops;
    NextHopThrift nhop;
    nhop.address() = toBinaryAddress(IPAddress(kNhopAddrA));
    nexthops.push_back(nhop);
    flowEntry->setEnabled(true);
    flowEntry->setCounterID(kCounterID);
    flowEntry->setNextHops(nexthops);
    flowEntry->setResolvedNextHops(nexthops);
    return flowEntry;
  }

  FlowEntry makeFlow(
      std::string dstIp,
      std::string nhop = kNhopAddrA,
      std::string ifname = "fboss1",
      std::string counterID = kCounterID,
      int prefixLength = 64) {
    FlowEntry flowEntry;
    flowEntry.flow()->srcPort() = 100;
    flowEntry.flow()->dstPrefix() = ipPrefix(dstIp, prefixLength);
    flowEntry.counterID() = counterID;
    flowEntry.nextHops()->resize(1);
    flowEntry.nextHops()[0].address() = toBinaryAddress(IPAddress(nhop));
    flowEntry.nextHops()[0].address()->ifName() = ifname;
    return flowEntry;
  }

  void verifyFlowEntry(
      std::shared_ptr<TeFlowEntry> entry,
      std::string nhop = kNhopAddrA,
      std::string counterID = kCounterID,
      std::string ifname = "fboss1") {
    EXPECT_NE(entry, nullptr);
    EXPECT_TRUE(entry->getEnabled());
    EXPECT_EQ(*entry->getCounterID(), counterID);
    EXPECT_EQ(entry->getNextHops()->size(), 1);
    auto expectedNhop = toBinaryAddress(IPAddress(nhop));
    expectedNhop.ifName() = ifname;
    EXPECT_EQ(
        entry->getNextHops()->cref(0)->toThrift().address(), expectedNhop);
    EXPECT_EQ(entry->getResolvedNextHops()->size(), 1);
    EXPECT_EQ(
        entry->getResolvedNextHops()->cref(0)->toThrift().address(),
        expectedNhop);
  }

  void updateState(folly::StringPiece name, StateUpdateFn func) {
    sw_->updateStateBlocking(name, func);
  }
  std::unique_ptr<HwTestHandle> handle_;
  SwSwitch* sw_;
};

TEST_F(TeFlowTest, SerDeserFlowEntry) {
  auto flowEntry = makeFlowEntry("100::");
  auto serialized = flowEntry->toThrift();
  auto entryBack = std::make_shared<TeFlowEntry>(serialized);
  EXPECT_TRUE(*flowEntry == *entryBack);
}

TEST_F(TeFlowTest, serDeserSwitchState) {
  auto state = this->sw_->getState();
  state->modify(&state);
  auto flowTable = state->getTeFlowTable().get()->modify(&state);
  auto flowEntry1 = makeFlow("100::");
  auto flowId1 = makeFlowKey("100::");
  auto teFlowEntry1 = TeFlowEntry::createTeFlowEntry(flowEntry1);
  teFlowEntry1->resolve(state);
  flowTable->addTeFlowEntry(teFlowEntry1);
  auto flowEntry2 = makeFlow("101::");
  auto flowId2 = makeFlowKey("101::");
  auto teFlowEntry2 = TeFlowEntry::createTeFlowEntry(flowEntry2);
  teFlowEntry2->resolve(state);
  flowTable->addTeFlowEntry(teFlowEntry2);
  EXPECT_EQ(state->getTeFlowTable()->size(), 2);
  auto serialized = state->toThrift();
  auto stateBack = SwitchState::fromThrift(serialized);
  EXPECT_EQ(state->toThrift(), stateBack->toThrift());
}

TEST_F(TeFlowTest, AddDeleteTeFlow) {
  auto state = sw_->getState();
  state->modify(&state);
  auto flowTable = state->getTeFlowTable().get()->modify(&state);
  auto flowId = makeFlowKey("100::");

  // No NDP entry should result in throw
  auto flowEntry = makeFlow("100::", kNhopAddrA, "fboss55");
  auto teFlowEntry = TeFlowEntry::createTeFlowEntry(flowEntry);
  EXPECT_THROW(teFlowEntry->resolve(state), FbossError);
  flowTable->addTeFlowEntry(teFlowEntry);

  // Valid nexthop should not throw
  flowEntry = makeFlow("100::");
  teFlowEntry = TeFlowEntry::createTeFlowEntry(flowEntry);
  EXPECT_NO_THROW(teFlowEntry->resolve(state));
  flowTable->changeTeFlowEntry(teFlowEntry);
  verifyFlowEntry(teFlowEntry);

  // change flow entry
  flowEntry.nextHops()[0].address() = toBinaryAddress(IPAddress(kNhopAddrB));
  flowEntry.nextHops()[0].address()->ifName() = "fboss55";
  flowEntry.counterID() = "counter1";
  teFlowEntry = TeFlowEntry::createTeFlowEntry(flowEntry);
  teFlowEntry->resolve(state);
  EXPECT_NO_THROW(flowTable->changeTeFlowEntry(teFlowEntry));
  verifyFlowEntry(teFlowEntry, kNhopAddrB, "counter1", "fboss55");
  // delete the entry
  flowTable->removeTeFlowEntry(flowId);
  EXPECT_EQ(flowTable->getTeFlowIf(flowId), nullptr);
}

TEST_F(TeFlowTest, NextHopResolution) {
  auto state = sw_->getState();
  auto flowId = makeFlowKey("100::");
  auto flowEntry = makeFlow("100::");

  updateState("add te flow", [&](const auto& state) {
    auto newState = state->clone();
    auto flowTable = std::make_shared<TeFlowTable>();
    auto teFlowEntry = TeFlowEntry::createTeFlowEntry(flowEntry);
    teFlowEntry->resolve(newState);
    flowTable->addTeFlowEntry(teFlowEntry);
    newState->resetTeFlowTable(flowTable);
    return newState;
  });
  auto tableEntry = sw_->getState()->getTeFlowTable()->getTeFlowIf(flowId);
  verifyFlowEntry(tableEntry);

  // test neighbor removal
  sw_->getNeighborUpdater()->flushEntry(kVlanA, folly::IPAddressV6(kNhopAddrA));
  sw_->getNeighborUpdater()->waitForPendingUpdates();
  waitForBackgroundThread(sw_);
  waitForStateUpdates(sw_);
  tableEntry = sw_->getState()->getTeFlowTable()->getTeFlowIf(flowId);
  EXPECT_EQ(tableEntry->getEnabled(), false);
  EXPECT_EQ(tableEntry->getResolvedNextHops()->size(), 0);

  // add back the neighbor entry
  sw_->getNeighborUpdater()->receivedNdpMine(
      kVlanA,
      folly::IPAddressV6(kNhopAddrA),
      kMacAddress,
      PortDescriptor(kPortIDA),
      ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
      0);
  sw_->getNeighborUpdater()->waitForPendingUpdates();
  waitForBackgroundThread(sw_);
  waitForStateUpdates(sw_);
  tableEntry = sw_->getState()->getTeFlowTable()->getTeFlowIf(flowId);
  EXPECT_EQ(tableEntry->getEnabled(), true);
  verifyFlowEntry(tableEntry);
}

TEST_F(TeFlowTest, TeFlowStats) {
  auto state = sw_->getState();
  auto flowEntries = {
      makeFlow("100::"),
      makeFlow("101::"),
      makeFlow("102::"),
      makeFlow("103::", kNhopAddrB, "fboss55"),
      makeFlow("104::", kNhopAddrB, "fboss55")};

  CounterCache counters(sw_);

  updateState("add te flow", [&](const auto& state) {
    auto newState = state->clone();
    auto flowTable = std::make_shared<TeFlowTable>();
    for (const auto& flowEntry : flowEntries) {
      auto teFlowEntry = TeFlowEntry::createTeFlowEntry(flowEntry);
      teFlowEntry->resolve(newState);
      flowTable->addTeFlowEntry(teFlowEntry);
    }
    newState->resetTeFlowTable(flowTable);
    return newState;
  });

  waitForStateUpdates(sw_);
  sw_->updateStats();
  counters.update();
  EXPECT_TRUE(counters.checkExist(SwitchStats::kCounterPrefix + "teflows"));
  EXPECT_EQ(counters.value(SwitchStats::kCounterPrefix + "teflows"), 5);
  EXPECT_EQ(
      counters.value(SwitchStats::kCounterPrefix + "teflows.inactive"), 0);

  // trigger ndp flush to disable NextHopB enentries
  sw_->getNeighborUpdater()->flushEntry(kVlanB, folly::IPAddressV6(kNhopAddrB));
  sw_->getNeighborUpdater()->waitForPendingUpdates();
  waitForBackgroundThread(sw_);
  waitForStateUpdates(sw_);
  sw_->updateStats();
  counters.update();
  EXPECT_TRUE(counters.checkExist(SwitchStats::kCounterPrefix + "teflows"));
  EXPECT_EQ(counters.value(SwitchStats::kCounterPrefix + "teflows"), 5);
  EXPECT_EQ(
      counters.value(SwitchStats::kCounterPrefix + "teflows.inactive"), 2);
}

TEST_F(TeFlowTest, TeFlowCounter) {
  auto state = sw_->getState();
  // 3 flow entries with 2 counters
  auto flowEntries = {
      makeFlow("100::", kNhopAddrA, "fboss1", "counter0"),
      makeFlow("101::", kNhopAddrA, "fboss1", "counter0"),
      makeFlow("102::", kNhopAddrA, "fboss1", "counter1")};

  updateState("add te flows", [&](const auto& state) {
    auto newState = state->clone();
    auto flowTable = std::make_shared<TeFlowTable>();
    for (const auto& flowEntry : flowEntries) {
      auto teFlowEntry = TeFlowEntry::createTeFlowEntry(flowEntry);
      teFlowEntry->resolve(newState);
      flowTable->addTeFlowEntry(teFlowEntry);
    }
    newState->resetTeFlowTable(flowTable);
    return newState;
  });

  int testByteCounterValue = 64;
  auto updateCounter = [&testByteCounterValue](auto counterID) {
    auto counter = std::make_unique<facebook::stats::MonotonicCounter>(
        counterID, facebook::fb303::SUM, facebook::fb303::RATE);
    auto now = duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch());
    counter->updateValue(now, 0);
    now = duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch());
    counter->updateValue(now, testByteCounterValue);
  };
  updateCounter("counter0.bytes");
  updateCounter("counter1.bytes");
  CounterCache counters(sw_);
  counters.update();
  auto teFlowStats = sw_->getTeFlowStats();
  auto hwTeFlowStats = teFlowStats.hwTeFlowStats();
  EXPECT_EQ(hwTeFlowStats->size(), 2);
  EXPECT_NE(teFlowStats.timestamp(), 0);
  for (const auto& stat : *hwTeFlowStats) {
    EXPECT_EQ(*stat.second.bytes(), testByteCounterValue);
  }
}
