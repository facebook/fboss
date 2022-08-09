// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/TeFlowEntry.h"

using namespace facebook::fboss;

using facebook::network::toBinaryAddress;
using facebook::network::thrift::BinaryAddress;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::StringPiece;

namespace {
static TeCounterID kCounterID("counter0");
} // namespace

IpPrefix ipPrefix(StringPiece ip, int length) {
  IpPrefix result;
  result.ip() = toBinaryAddress(IPAddress(ip));
  result.prefixLength() = length;
  return result;
}

TeFlow makeFlowKey(std::string dstIp) {
  TeFlow flow;
  flow.srcPort() = 100;
  flow.dstPrefix() = ipPrefix(dstIp, 64);
  return flow;
}

std::shared_ptr<TeFlowEntry> makeFlowEntry(std::string dstIp) {
  auto flow = makeFlowKey(dstIp);
  auto flowEntry = std::make_shared<TeFlowEntry>(flow);
  std::vector<NextHopThrift> nexthops;
  NextHopThrift nhop;
  nhop.address() = toBinaryAddress(IPAddress("1::1"));
  nexthops.push_back(nhop);
  flowEntry->setEnabled(true);
  flowEntry->setCounterID(kCounterID);
  flowEntry->setNextHops(nexthops);
  flowEntry->setResolvedNextHops(nexthops);
  return flowEntry;
}

FlowEntry makeFlow(std::string dstIp) {
  FlowEntry flowEntry;
  flowEntry.flow()->srcPort() = 100;
  flowEntry.flow()->dstPrefix() = ipPrefix(dstIp, 64);
  flowEntry.counterID() = kCounterID;
  flowEntry.nextHops()->resize(1);
  flowEntry.nextHops()[0].address() = toBinaryAddress(IPAddress("1::1"));
  return flowEntry;
}

void verifyFlowEntry(
    std::shared_ptr<TeFlowEntry> entry,
    std::string nhop = "1::1",
    std::string counterID = kCounterID) {
  EXPECT_NE(entry, nullptr);
  EXPECT_TRUE(entry->getEnabled());
  EXPECT_EQ(entry->getCounterID(), counterID);
  EXPECT_EQ(entry->getNextHops().size(), 1);
  EXPECT_EQ(
      entry->getNextHops()[0].address(), toBinaryAddress(IPAddress(nhop)));
  EXPECT_EQ(entry->getResolvedNextHops().size(), 1);
  EXPECT_EQ(
      entry->getResolvedNextHops()[0].address(),
      toBinaryAddress(IPAddress(nhop)));
}

TEST(TeFlowTest, SerDeserFlowEntry) {
  auto flowEntry = makeFlowEntry("100::");
  auto serialized = flowEntry->toFollyDynamic();
  auto entryBack = TeFlowEntry::fromFollyDynamic(serialized);
  EXPECT_TRUE(*flowEntry == *entryBack);
}

TEST(TeFlowTest, serDeserSwitchState) {
  auto state = std::make_shared<SwitchState>();
  auto flowTable = std::make_shared<TeFlowTable>();
  auto flowEntry1 = makeFlow("100::");
  flowTable->addTeFlowEntry(&state, flowEntry1);
  auto flowEntry2 = makeFlow("101::");
  flowTable->addTeFlowEntry(&state, flowEntry2);
  state->resetTeFlowTable(flowTable);
  EXPECT_EQ(state->getTeFlowTable()->size(), 2);
  auto serialized = state->toFollyDynamic();
  auto stateBack = SwitchState::fromFollyDynamic(serialized);
  EXPECT_EQ(*state, *stateBack);
}

TEST(TeFlowTest, AddDeleteTeFlow) {
  auto state = std::make_shared<SwitchState>();
  auto flowTable = std::make_shared<TeFlowTable>();
  auto flowId = makeFlowKey("100::");
  auto flowEntry = makeFlow("100::");
  flowTable->addTeFlowEntry(&state, flowEntry);
  auto tableEntry = flowTable->getTeFlowIf(flowId);
  verifyFlowEntry(tableEntry);
  // change flow entry
  flowEntry.nextHops()[0].address() = toBinaryAddress(IPAddress("2::1"));
  flowEntry.counterID() = "counter1";
  flowTable->addTeFlowEntry(&state, flowEntry);
  tableEntry = flowTable->getTeFlowIf(flowId);
  verifyFlowEntry(tableEntry, "2::1", "counter1");
  // delete the entry
  flowTable->removeTeFlowEntry(&state, flowId);
  EXPECT_EQ(flowTable->getTeFlowIf(flowId), nullptr);
}
