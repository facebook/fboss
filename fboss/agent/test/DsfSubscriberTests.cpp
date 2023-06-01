/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/DsfSubscriber.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <optional>

#include "fboss/agent/HwSwitchMatcher.h"

using namespace facebook::fboss;

namespace {
constexpr auto kRemoteSwitchId = 42;
constexpr auto kSysPortRangeMin = 1000;
std::shared_ptr<SystemPortMap> makeSysPorts() {
  auto sysPorts = std::make_shared<SystemPortMap>();
  for (auto sysPortId = kSysPortRangeMin + 1; sysPortId < kSysPortRangeMin + 3;
       ++sysPortId) {
    sysPorts->addNode(makeSysPort(std::nullopt, sysPortId, kRemoteSwitchId));
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
    rifs->addNode(rif);
  }
  return rifs;
}
} // namespace

namespace facebook::fboss {
class DsfSubscriberTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto config = testConfigA(cfg::SwitchType::VOQ);
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
    // Create a separate instance of DsfSubscriber (vs
    // using one from SwSwitch) for ease of testing.
    dsfSubscriber_ = std::make_unique<DsfSubscriber>(sw_);
  }

 protected:
  SwSwitch* sw_;
  std::unique_ptr<HwTestHandle> handle_;
  std::unique_ptr<DsfSubscriber> dsfSubscriber_;
};

TEST_F(DsfSubscriberTest, scheduleUpdate) {
  auto sysPorts = makeSysPorts();
  auto rifs = makeRifs(sysPorts.get());
  dsfSubscriber_->scheduleUpdate(
      sysPorts, rifs, "switch", SwitchID(kRemoteSwitchId));
  // Don't wait for state update to mimic async scheduling of
  // state updates.
}

TEST_F(DsfSubscriberTest, setupNeighbors) {
  auto updateAndCompareTables = [this](
                                    const auto& sysPorts,
                                    const auto& rifs,
                                    bool publishState,
                                    bool noNeighbors = false) {
    if (publishState) {
      rifs->publish();
    }

    // dsfSubscriber_->scheduleUpdate is expected to set isLocal to False,
    // and rest of the structure should remain the same.
    auto expectedRifs = InterfaceMap(rifs->toThrift());
    for (auto intfIter : expectedRifs) {
      auto& intf = intfIter.second;
      for (auto& ndpEntry : *intf->getNdpTable()) {
        ndpEntry.second->setIsLocal(false);
      }
      for (auto& arpEntry : *intf->getArpTable()) {
        arpEntry.second->setIsLocal(false);
      }
    }

    dsfSubscriber_->scheduleUpdate(
        sysPorts, rifs, "switch", SwitchID(kRemoteSwitchId));
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
        expectedRifs.toThrift(),
        sw_->getState()->getRemoteInterfaces()->getAllNodes()->toThrift());

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
