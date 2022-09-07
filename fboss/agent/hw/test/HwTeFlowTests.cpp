/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/IPAddress.h>
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestTeFlowUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/TeFlowEntry.h"

namespace facebook::fboss {

using facebook::network::toBinaryAddress;
using folly::IPAddress;
using folly::StringPiece;

namespace {
static TeCounterID kCounterID("counter0");
static std::string kNhopAddrA("2620:0:1cfe:face:b00c::4");
static std::string kNhopAddrB("2620:0:1cfe:face:b00c::5");
static std::string kIfName1("fboss2000");
static std::string kIfName2("fboss2001");
} // namespace

class HwTeFlowTest : public HwLinkStateDependentTest {
 protected:
  void SetUp() override {
    FLAGS_enable_exact_match = true;
    HwLinkStateDependentTest::SetUp();
  }

  cfg::SwitchConfig initialConfig() const override {
    std::vector<PortID> ports = {
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
    };
    auto cfg = utility::onePortPerVlanConfig(
        getHwSwitch(), std::move(ports), cfg::PortLoopbackMode::MAC);
    return cfg;
  }

  bool skipTest() {
    return !getPlatform()->getAsic()->isSupported(HwAsic::Feature::EXACT_MATCH);
  }

  IpPrefix ipPrefix(StringPiece ip, int length) {
    IpPrefix result;
    result.ip() = toBinaryAddress(IPAddress(ip));
    result.prefixLength() = length;
    return result;
  }

  TeFlow makeFlowKey(std::string dstIp) {
    TeFlow flow;
    flow.srcPort() = 100;
    flow.dstPrefix() = ipPrefix(dstIp, 56);
    return flow;
  }

  std::shared_ptr<TeFlowEntry> makeFlowEntry(
      std::string dstIp,
      std::string nhopAdd = kNhopAddrA,
      std::string ifName = kIfName1) {
    auto flow = makeFlowKey(dstIp);
    auto flowEntry = std::make_shared<TeFlowEntry>(flow);
    std::vector<NextHopThrift> nexthops;
    NextHopThrift nhop;
    nhop.address() = toBinaryAddress(IPAddress(nhopAdd));
    nhop.address()->ifName() = ifName;
    nexthops.push_back(nhop);
    flowEntry->setEnabled(true);
    flowEntry->setCounterID(kCounterID);
    flowEntry->setNextHops(nexthops);
    flowEntry->setResolvedNextHops(nexthops);
    return flowEntry;
  }

  void addFlowEntry(std::shared_ptr<TeFlowEntry>& flowEntry) {
    auto state = this->getProgrammedState()->clone();
    auto teFlows = state->getTeFlowTable()->modify(&state);
    teFlows->addNode(flowEntry);
    this->applyNewState(state);
  }

  void deleteFlowEntry(std::shared_ptr<TeFlowEntry>& flowEntry) {
    auto teFlows = this->getProgrammedState()->getTeFlowTable()->clone();
    teFlows->removeNode(flowEntry);
    auto newState = this->getProgrammedState()->clone();
    newState->resetTeFlowTable(teFlows);
    this->applyNewState(newState);
  }

  void modifyFlowEntry(std::string dstIp) {
    auto flow = makeFlowKey(dstIp);
    auto teFlows = this->getProgrammedState()->getTeFlowTable()->clone();
    auto newFlowEntry = makeFlowEntry(dstIp, kNhopAddrB, kIfName2);
    teFlows->updateNode(newFlowEntry);
    auto newState = this->getProgrammedState()->clone();
    newState->resetTeFlowTable(teFlows);
    this->applyNewState(newState);
  }
};

TEST_F(HwTeFlowTest, VerifyTeFlowGroupEnable) {
  if (this->skipTest()) {
    return;
  }

  auto verify = [&]() {
    EXPECT_TRUE(utility::validateTeFlowGroupEnabled(getHwSwitch()));
  };

  verify();
}

TEST_F(HwTeFlowTest, validateAddDeleteTeFlow) {
  if (this->skipTest()) {
    return;
  }

  auto flowEntry1 = makeFlowEntry("100::");
  this->addFlowEntry(flowEntry1);

  EXPECT_EQ(utility::getNumTeFlowEntries(getHwSwitch()), 1);
  utility::checkSwHwTeFlowMatch(
      getHwSwitch(), getProgrammedState(), makeFlowKey("100::"));

  auto flowEntry2 = makeFlowEntry("101::");
  this->addFlowEntry(flowEntry2);

  EXPECT_EQ(utility::getNumTeFlowEntries(getHwSwitch()), 2);
  utility::checkSwHwTeFlowMatch(
      getHwSwitch(), getProgrammedState(), makeFlowKey("101::"));

  this->modifyFlowEntry("100::");
  EXPECT_EQ(utility::getNumTeFlowEntries(getHwSwitch()), 2);
  utility::checkSwHwTeFlowMatch(
      getHwSwitch(), getProgrammedState(), makeFlowKey("100::"));

  this->deleteFlowEntry(flowEntry1);

  EXPECT_EQ(utility::getNumTeFlowEntries(getHwSwitch()), 1);
}

} // namespace facebook::fboss
