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

HwSwitchMatcher scope() {
  return HwSwitchMatcher{std::unordered_set<SwitchID>{SwitchID(0)}};
}
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

  void
  fillNexthops(FlowEntry& flowEntry, std::string& nhop, std::string& ifname) {
    std::vector<NextHopThrift> nexthops;
    NextHopThrift nexthop;
    nexthop.address() = toBinaryAddress(IPAddress(nhop));
    nexthop.address()->ifName() = ifname;
    nexthops.push_back(nexthop);
    flowEntry.nexthops() = nexthops;
  }

  // TODO remove this later
  void
  fillNextHops(FlowEntry& flowEntry, std::string& nhop, std::string& ifname) {
    flowEntry.nextHops()->resize(1);
    flowEntry.nextHops()[0].address() = toBinaryAddress(IPAddress(nhop));
    flowEntry.nextHops()[0].address()->ifName() = ifname;
  }

  FlowEntry makeFlow(
      std::string dstIp,
      std::string nhop = kNhopAddrA,
      std::string ifname = "fboss1",
      std::string counterID = kCounterID,
      int prefixLength = 64,
      bool useNewFormat = true) {
    FlowEntry flowEntry;
    flowEntry.flow()->srcPort() = 100;
    flowEntry.flow()->dstPrefix() = ipPrefix(dstIp, prefixLength);
    flowEntry.counterID() = counterID;
    if (useNewFormat) {
      fillNexthops(flowEntry, nhop, ifname);
    } else {
      fillNextHops(flowEntry, nhop, ifname);
    }
    return flowEntry;
  }

  void verifyFlowEntry(
      std::shared_ptr<TeFlowEntry> entry,
      std::string nhop = kNhopAddrA,
      std::string counterID = kCounterID,
      std::string ifname = "fboss1") {
    EXPECT_NE(entry, nullptr);
    EXPECT_TRUE(entry->getEnabled());
    EXPECT_TRUE(*entry->getStatEnabled());
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
  flowTable->addNode(teFlowEntry1, scope());
  auto flowEntry2 = makeFlow("101::");
  auto flowId2 = makeFlowKey("101::");
  auto teFlowEntry2 = TeFlowEntry::createTeFlowEntry(flowEntry2);
  teFlowEntry2->resolve(state);
  flowTable->addNode(teFlowEntry2, scope());
  EXPECT_EQ(state->getTeFlowTable()->numNodes(), 2);
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
  flowTable->addNode(teFlowEntry, scope());

  // Valid nexthop should not throw
  flowEntry = makeFlow("100::");
  teFlowEntry = TeFlowEntry::createTeFlowEntry(flowEntry);
  EXPECT_NO_THROW(teFlowEntry->resolve(state));
  flowTable->updateNode(teFlowEntry, scope());
  verifyFlowEntry(teFlowEntry);

  // change flow entry
  auto flowEntry1 = makeFlow("100::", kNhopAddrB, "fboss2000");
  flowEntry1.counterID() = "counter1";
  teFlowEntry = TeFlowEntry::createTeFlowEntry(flowEntry1);
  EXPECT_THROW(teFlowEntry->resolve(state), FbossError);

  // change with interface which exists
  auto flowEntry2 = makeFlow("100::", kNhopAddrB, "fboss55");
  flowEntry2.counterID() = "counter1";
  teFlowEntry = TeFlowEntry::createTeFlowEntry(flowEntry2);
  teFlowEntry->resolve(state);
  EXPECT_NO_THROW(flowTable->updateNode(teFlowEntry, scope()));
  verifyFlowEntry(teFlowEntry, kNhopAddrB, "counter1", "fboss55");
  // delete the entry
  flowTable->removeNode(getTeFlowStr(flowId));
  EXPECT_EQ(flowTable->getNodeIf(getTeFlowStr(flowId)), nullptr);

  // Add an entry with old format and check
  auto flowEntry3 =
      makeFlow("101::", kNhopAddrB, "fboss55", "counter1", 64, false);
  teFlowEntry = TeFlowEntry::createTeFlowEntry(flowEntry3);
  teFlowEntry->resolve(state);
  flowTable->addNode(teFlowEntry, scope());
  verifyFlowEntry(teFlowEntry, kNhopAddrB, "counter1", "fboss55");
}

TEST_F(TeFlowTest, NextHopResolution) {
  auto state = sw_->getState();
  auto flowId = makeFlowKey("100::");
  auto flowEntry = makeFlow("100::");

  updateState("add te flow", [&](const auto& state) {
    auto newState = state->clone();
    auto flowTable = newState->getTeFlowTable()->modify(&newState);
    auto teFlowEntry = TeFlowEntry::createTeFlowEntry(flowEntry);
    teFlowEntry->resolve(newState);
    flowTable->addNode(teFlowEntry, scope());
    return newState;
  });
  auto tableEntry =
      sw_->getState()->getTeFlowTable()->getNodeIf(getTeFlowStr(flowId));
  verifyFlowEntry(tableEntry);

  // test neighbor removal
  sw_->getNeighborUpdater()->flushEntry(kVlanA, folly::IPAddressV6(kNhopAddrA));
  sw_->getNeighborUpdater()->waitForPendingUpdates();
  waitForBackgroundThread(sw_);
  waitForStateUpdates(sw_);
  tableEntry =
      sw_->getState()->getTeFlowTable()->getNodeIf(getTeFlowStr(flowId));
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
  tableEntry =
      sw_->getState()->getTeFlowTable()->getNodeIf(getTeFlowStr(flowId));
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
    auto flowTable = newState->getTeFlowTable()->modify(&newState);
    for (const auto& flowEntry : flowEntries) {
      auto teFlowEntry = TeFlowEntry::createTeFlowEntry(flowEntry);
      teFlowEntry->resolve(newState);
      flowTable->addNode(teFlowEntry, scope());
    }
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
    auto flowTable = newState->getTeFlowTable()->modify(&newState);
    for (const auto& flowEntry : flowEntries) {
      auto teFlowEntry = TeFlowEntry::createTeFlowEntry(flowEntry);
      teFlowEntry->resolve(newState);
      flowTable->addNode(teFlowEntry, scope());
    }
    return newState;
  });

  EXPECT_EQ(sw_->getState()->getTeFlowTable()->numNodes(), 3);
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

TEST_F(TeFlowTest, addRemoveTeFlow) {
  TeFlowSyncer teFlowSyncer;
  auto state = sw_->getState();
  auto teFlowEntries = std::make_unique<std::vector<FlowEntry>>();
  auto flowEntry = makeFlow("100::1");
  teFlowEntries->emplace_back(flowEntry);
  updateState("add te flow", [&](const auto& state) {
    auto newState = teFlowSyncer.programFlowEntries(
        scope(), state, *teFlowEntries, {}, false);
    return newState;
  });
  state = sw_->getState();
  auto teFlowTable = state->getTeFlowTable();
  auto verifyEntry = [&teFlowTable](
                         const auto& flow,
                         const auto& nhop,
                         const auto& counter,
                         const auto& intf) {
    EXPECT_EQ(teFlowTable->numNodes(), 1);
    auto tableEntry = teFlowTable->getNodeIf(getTeFlowStr(flow));
    EXPECT_NE(tableEntry, nullptr);
    EXPECT_EQ(*tableEntry->getCounterID(), counter);
    EXPECT_EQ(tableEntry->getNextHops()->size(), 1);
    auto expectedNhop = toBinaryAddress(IPAddress(nhop));
    expectedNhop.ifName() = intf;
    auto nexthop = tableEntry->getNextHops()->cref(0)->toThrift();
    EXPECT_EQ(*nexthop.address(), expectedNhop);
    EXPECT_EQ(tableEntry->getResolvedNextHops()->size(), 1);
    auto resolveNexthop =
        tableEntry->getResolvedNextHops()->cref(0)->toThrift();
    EXPECT_EQ(*resolveNexthop.address(), expectedNhop);
  };
  verifyEntry(*flowEntry.flow(), kNhopAddrA, "counter0", "fboss1");

  // modify the entry
  auto flowEntry2 = makeFlow("100::1", kNhopAddrB, "fboss55", "counter1");
  auto newFlowEntries = std::make_unique<std::vector<FlowEntry>>();
  newFlowEntries->emplace_back(flowEntry2);
  updateState("modify te flow", [&](const auto& state) {
    auto newState = teFlowSyncer.programFlowEntries(
        scope(), state, *newFlowEntries, {}, false);
    return newState;
  });
  state = sw_->getState();
  teFlowTable = state->getTeFlowTable();
  verifyEntry(*flowEntry.flow(), kNhopAddrB, "counter1", "fboss55");

  auto teFlows = std::make_unique<std::vector<TeFlow>>();
  teFlows->emplace_back(*flowEntry.flow());
  updateState("delete te flow", [&](const auto& state) {
    auto newState =
        teFlowSyncer.programFlowEntries(scope(), state, {}, *teFlows, false);
    return newState;
  });
  state = sw_->getState();
  teFlowTable = state->getTeFlowTable();
  auto tableEntry = teFlowTable->getNodeIf(getTeFlowStr(*flowEntry.flow()));
  EXPECT_EQ(tableEntry, nullptr);

  // add flows in bulk
  auto testPrefixes = {"100::1", "101::1", "102::1", "103::1"};
  auto bulkEntries = std::make_unique<std::vector<FlowEntry>>();
  for (const auto& prefix : testPrefixes) {
    bulkEntries->emplace_back(makeFlow(prefix));
  }
  updateState("bulk add te flow", [&](const auto& state) {
    auto newState = teFlowSyncer.programFlowEntries(
        scope(), state, *bulkEntries, {}, false);
    return newState;
  });
  state = sw_->getState();
  teFlowTable = state->getTeFlowTable();
  EXPECT_EQ(teFlowTable->numNodes(), 4);

  // bulk delete
  auto flowsToDelete = {"100::1", "101::1"};
  auto deletionFlows = std::make_unique<std::vector<TeFlow>>();
  for (const auto& prefix : flowsToDelete) {
    TeFlow flow;
    flow.dstPrefix() = ipPrefix(prefix, 64);
    flow.srcPort() = 100;
    deletionFlows->emplace_back(flow);
  }
  updateState("bulk delete te flow", [&](const auto& state) {
    auto newState = teFlowSyncer.programFlowEntries(
        scope(), state, {}, *deletionFlows, false);
    return newState;
  });
  state = sw_->getState();
  teFlowTable = state->getTeFlowTable();
  EXPECT_EQ(teFlowTable->numNodes(), 2);
  for (const auto& prefix : flowsToDelete) {
    TeFlow flow;
    flow.dstPrefix() = ipPrefix(prefix, 64);
    flow.srcPort() = 100;
    EXPECT_EQ(teFlowTable->getNodeIf(getTeFlowStr(flow)), nullptr);
  }
}

TEST_F(TeFlowTest, syncTeFlows) {
  TeFlowSyncer teFlowSyncer;
  auto state = sw_->getState();
  auto initalPrefixes = {"100::1", "101::1", "102::1", "103::1"};
  auto flowEntries = std::make_unique<std::vector<FlowEntry>>();
  for (const auto& prefix : initalPrefixes) {
    auto flowEntry = makeFlow(prefix);
    flowEntries->emplace_back(flowEntry);
  }

  updateState("add te flows", [&](const auto& state) {
    auto newState = state->clone();
    auto flowTable = newState->getTeFlowTable()->modify(&newState);
    for (const auto& flowEntry : *flowEntries) {
      auto teFlowEntry = TeFlowEntry::createTeFlowEntry(flowEntry);
      teFlowEntry->resolve(newState);
      flowTable->addNode(teFlowEntry, scope());
    }
    return newState;
  });

  state = sw_->getState();
  auto teFlowTable = state->getTeFlowTable();
  EXPECT_EQ(teFlowTable->numNodes(), 4);

  // Ensure that all entries are created
  for (const auto& prefix : initalPrefixes) {
    TeFlow flow;
    flow.dstPrefix() = ipPrefix(prefix, 64);
    flow.srcPort() = 100;
    auto tableEntry = teFlowTable->getNodeIf(getTeFlowStr(flow));
    EXPECT_NE(tableEntry, nullptr);
  }

  TeFlow teFlow;
  teFlow.dstPrefix() = ipPrefix("100::1", 64);
  teFlow.srcPort() = 100;
  auto teflowEntryBeforeSync = teFlowTable->getNodeIf(getTeFlowStr(teFlow));
  auto syncPrefixes = {"100::1", "101::1", "104::1"};
  auto syncFlowEntries = std::make_unique<std::vector<FlowEntry>>();
  for (const auto& prefix : syncPrefixes) {
    auto flowEntry = makeFlow(prefix);
    syncFlowEntries->emplace_back(flowEntry);
  }
  // sync teflow
  updateState("sync te flow", [&](const auto& state) {
    auto newState = teFlowSyncer.programFlowEntries(
        scope(), state, *syncFlowEntries, {}, true);
    return newState;
  });

  state = sw_->getState();
  teFlowTable = state->getTeFlowTable();
  EXPECT_EQ(teFlowTable->numNodes(), 3);
  // Ensure that newly added entries are present
  for (const auto& prefix : syncPrefixes) {
    TeFlow flow;
    flow.dstPrefix() = ipPrefix(prefix, 64);
    flow.srcPort() = 100;
    auto tableEntry = teFlowTable->getNodeIf(getTeFlowStr(flow));
    EXPECT_NE(tableEntry, nullptr);
  }
  // Ensure that missing entries are removed
  for (const auto& prefix : {"102::1", "103::1"}) {
    TeFlow flow;
    flow.dstPrefix() = ipPrefix(prefix, 64);
    flow.srcPort() = 100;
    auto tableEntry = teFlowTable->getNodeIf(getTeFlowStr(flow));
    EXPECT_EQ(tableEntry, nullptr);
  }
  // Ensure that pointer to entries and contents are same
  auto teflowEntryAfterSync = teFlowTable->getNodeIf(getTeFlowStr(teFlow));
  EXPECT_EQ(teflowEntryBeforeSync, teflowEntryAfterSync);
  EXPECT_EQ(*teflowEntryBeforeSync, *teflowEntryAfterSync);
  // Sync with no change in entries and verify table is same
  auto syncFlowEntries2 = std::make_unique<std::vector<FlowEntry>>();
  for (const auto& prefix : syncPrefixes) {
    auto flowEntry = makeFlow(prefix);
    syncFlowEntries2->emplace_back(flowEntry);
  }
  updateState("sync same te flow", [&](const auto& state) {
    auto newState = teFlowSyncer.programFlowEntries(
        scope(), state, *syncFlowEntries2, {}, true);
    return newState;
  });
  state = sw_->getState();
  auto teFlowTableAfterSync = state->getTeFlowTable();
  // Ensure teflow table pointers and contents are same
  EXPECT_EQ(teFlowTable, teFlowTableAfterSync);
  EXPECT_EQ(*teFlowTable, *teFlowTableAfterSync);
  // Update an entry and check the pointer and content changed
  teFlow.dstPrefix() = ipPrefix("104::1", 64);
  teFlow.srcPort() = 100;
  teflowEntryBeforeSync = teFlowTable->getNodeIf(getTeFlowStr(teFlow));
  auto updateEntries = std::make_unique<std::vector<FlowEntry>>();
  auto flowEntry1 = makeFlow("100::1");
  auto flowEntry2 = makeFlow("104::1", kNhopAddrA, "fboss1", "counter1");
  updateEntries->emplace_back(flowEntry1);
  updateEntries->emplace_back(flowEntry2);
  updateState("sync te flow for update", [&](const auto& state) {
    auto newState = teFlowSyncer.programFlowEntries(
        scope(), state, *updateEntries, {}, true);
    return newState;
  });
  state = sw_->getState();
  teFlowTable = state->getTeFlowTable();
  teflowEntryAfterSync = teFlowTable->getNodeIf(getTeFlowStr(teFlow));
  // Ensure that pointer to entries and contents are different
  EXPECT_NE(teflowEntryBeforeSync, teflowEntryAfterSync);
  EXPECT_NE(*teflowEntryBeforeSync, *teflowEntryAfterSync);
  // sync flows with no entries
  auto nullFlowEntries = std::make_unique<std::vector<FlowEntry>>();
  updateState("sync null flow", [&](const auto& state) {
    auto newState = teFlowSyncer.programFlowEntries(
        scope(), state, *nullFlowEntries, {}, true);
    return newState;
  });
  state = sw_->getState();
  teFlowTable = state->getTeFlowTable();
  EXPECT_EQ(teFlowTable->numNodes(), 0);
}
