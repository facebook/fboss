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
std::shared_ptr<MultiInterfaceMap> makeRifs(const SystemPortMap* sysPorts) {
  auto multiRifs = std::make_shared<MultiInterfaceMap>();
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
  multiRifs->addNode(HwSwitchMatcher::defaultHwSwitchMatcherKey(), rifs);
  return multiRifs;
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
                                    const std::shared_ptr<MultiInterfaceMap>&
                                        rifs,
                                    bool publishState,
                                    bool noNeighbors = false) {
    if (publishState) {
      rifs->publish();
    }

    // dsfSubscriber_->scheduleUpdate is expected to set isLocal to False,
    // and rest of the structure should remain the same.
    auto expectedRifs = InterfaceMap(
        rifs->cref(HwSwitchMatcher::defaultHwSwitchMatcherKey())->toThrift());
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
        sw_->getState()->getRemoteSystemPorts()->toThrift());
    EXPECT_EQ(
        expectedRifs.toThrift(),
        sw_->getState()->getRemoteInterfaces()->toThrift());

    // neighbor entries are modified to set isLocal=false
    // Thus, if neighbor table is non-empty, programmed vs. actually
    // programmed would be unequal for published state.
    // for unpublished state, the passed state would be modified, and thus,
    // programmed vs actually programmed state would be equal.
    EXPECT_TRUE(
        rifs->cref(HwSwitchMatcher::defaultHwSwitchMatcherKey())->toThrift() !=
            sw_->getState()->getRemoteInterfaces()->toThrift() ||
        noNeighbors || !publishState);
  };

  auto makeNbrs = []() {
    state::NeighborEntries ndpTable, arpTable;
    std::map<std::string, int> ip2Rif = {
        {"fc00::1", kSysPortRangeMin + 1},
        {"fc01::1", kSysPortRangeMin + 2},
        {"10.0.1.1", kSysPortRangeMin + 1},
        {"10.0.2.1", kSysPortRangeMin + 2},
    };
    for (const auto& [ip, rif] : ip2Rif) {
      state::NeighborEntryFields nbr;
      nbr.ipaddress() = ip;
      nbr.mac() = "01:02:03:04:05:06";
      cfg::PortDescriptor port;
      port.portId() = rif;
      port.portType() = cfg::PortDescriptorType::SystemPort;
      nbr.portId() = port;
      nbr.interfaceId() = rif;
      nbr.isLocal() = true;
      folly::IPAddress ipAddr(ip);
      if (ipAddr.isV6()) {
        ndpTable.insert({ip, nbr});
      } else {
        arpTable.insert({ip, nbr});
      }
    }
    return std::make_pair(ndpTable, arpTable);
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
      rifs->ref(HwSwitchMatcher::defaultHwSwitchMatcherKey())
          ->ref(firstRif)
          ->setNdpTable(ndpTable);
      rifs->ref(HwSwitchMatcher::defaultHwSwitchMatcherKey())
          ->ref(firstRif)
          ->setArpTable(arpTable);
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
      rifs->ref(HwSwitchMatcher::defaultHwSwitchMatcherKey())
          ->ref(firstRif)
          ->setNdpTable(ndpTable);
      rifs->ref(HwSwitchMatcher::defaultHwSwitchMatcherKey())
          ->ref(firstRif)
          ->setArpTable(arpTable);
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
      rifs->ref(HwSwitchMatcher::defaultHwSwitchMatcherKey())
          ->ref(firstRif)
          ->setNdpTable(ndpTable);
      rifs->ref(HwSwitchMatcher::defaultHwSwitchMatcherKey())
          ->ref(firstRif)
          ->setArpTable(arpTable);
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
