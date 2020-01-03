// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include "fboss/agent/ResolvedNexthopMonitor.h"
#include "fboss/agent/ResolvedNexthopProbeScheduler.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/ICMPHdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/state/SwitchState.h"

#include "folly/io/Cursor.h"

#include <gtest/gtest.h>

namespace {
using namespace facebook::fboss;
using folly::IPAddressV4;
using folly::IPAddressV6;

const auto kPrefixV4 = RoutePrefixV4{IPAddressV4("10.0.10.0"), 24};
const std::array<IPAddressV4, 2> kNexthopsV4{IPAddressV4("10.0.10.1"),
                                             IPAddressV4("10.0.10.2")};
const auto kPrefixV6 = RoutePrefixV6{IPAddressV6("10:100::"), 64};
const std::array<IPAddressV6, 2> kNexthopsV6{IPAddressV6("10:100::1"),
                                             IPAddressV6("10:100::2")};
} // namespace

namespace facebook::fboss {

class ResolvedNexthopMonitorTest : public ::testing::Test {
 public:
  using StateUpdateFn = SwSwitch::StateUpdateFn;
  using Func = folly::Function<void()>;
  void SetUp() override {
    handle_ = createTestHandle(testStateA());
    sw_ = handle_->getSw();
  }

  void TearDown() override {
    waitForStateUpdates(sw_);
    waitForBackgroundThread(sw_);
  }

  template <typename AddrT>
  std::shared_ptr<SwitchState> addRoute(
      const std::shared_ptr<SwitchState>& state,
      const RoutePrefix<AddrT>& prefix,
      RouteNextHopSet nexthops,
      ClientID client = ClientID::OPENR) {
    auto routeTables = state->getRouteTables();
    RouteUpdater updater(routeTables);
    updater.addRoute(
        RouterID(0),
        prefix.network,
        prefix.mask,
        client,
        RouteNextHopEntry(nexthops, AdminDistance::MAX_ADMIN_DISTANCE));
    auto newRouteTables = updater.updateDone();
    newRouteTables->publish();
    auto newState = state->isPublished() ? state->clone() : state;
    newState->resetRouteTables(newRouteTables);
    return newState;
  }

  template <typename AddrT>
  std::shared_ptr<SwitchState> delRoute(
      const std::shared_ptr<SwitchState>& state,
      const RoutePrefix<AddrT>& prefix,
      ClientID client = ClientID::OPENR) {
    auto routeTables = state->getRouteTables();
    RouteUpdater updater(routeTables);
    updater.delRoute(RouterID(0), prefix.network, prefix.mask, client);
    auto newRouteTables = updater.updateDone();
    newRouteTables->publish();
    auto newState = state->isPublished() ? state->clone() : state;
    newState->resetRouteTables(newRouteTables);
    return newState;
  }

  std::shared_ptr<SwitchState> toggleIgnoredStateUpdate(
      const std::shared_ptr<SwitchState>& state) {
    auto newState = state->isPublished() ? state->clone() : state;
    auto mirrors = newState->getMirrors()->clone();
    auto mirror = newState->getMirrors()->getMirrorIf("mirror");
    if (!mirror) {
      mirror = std::make_shared<Mirror>("mirror", PortID(5), std::nullopt);
      mirrors->addNode(mirror);
    } else {
      mirrors->removeNode(mirror);
    }
    newState->resetMirrors(mirrors);
    return newState;
  }

  void updateState(folly::StringPiece name, StateUpdateFn func) {
    sw_->updateStateBlocking(name, func);
  }

  void runInUpdateEventBaseAndWait(Func func) {
    auto* evb = sw_->getUpdateEvb();
    evb->runInEventBaseThreadAndWait(std::move(func));
  }

  void schedulePendingStateUpdates() {
    runInUpdateEventBaseAndWait([]() {});
  }

 protected:
  std::unique_ptr<HwTestHandle> handle_;
  SwSwitch* sw_;
};

TEST_F(ResolvedNexthopMonitorTest, AddUnresolvedRoutes) {
  updateState(
      "unresolved route added", [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{UnresolvedNextHop(kNexthopsV4[0], 1),
                              UnresolvedNextHop(kNexthopsV4[1], 1)};
        return addRoute(state, kPrefixV4, nhops);
      });

  schedulePendingStateUpdates();
  auto* monitor = sw_->getResolvedNexthopMonitor();
  EXPECT_FALSE(monitor->probesScheduled());
}

TEST_F(ResolvedNexthopMonitorTest, RemoveUnresolvedRoutes) {
  updateState(
      "unresolved route added", [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{UnresolvedNextHop(kNexthopsV4[0], 1),
                              UnresolvedNextHop(kNexthopsV4[1], 1)};
        return addRoute(state, kPrefixV4, nhops);
      });

  updateState(
      "unresolved route removed",
      [=](const std::shared_ptr<SwitchState>& state) {
        return delRoute(state, kPrefixV4);
      });

  schedulePendingStateUpdates();
  auto* monitor = sw_->getResolvedNexthopMonitor();
  EXPECT_FALSE(monitor->probesScheduled());
}

TEST_F(ResolvedNexthopMonitorTest, AddResolvedRoutes) {
  updateState(
      "resolved route added", [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{
            UnresolvedNextHop(folly::IPAddressV4("10.0.0.22"), 1),
            UnresolvedNextHop(folly::IPAddressV4("10.0.55.22"), 1)};
        return addRoute(state, kPrefixV4, nhops);
      });

  schedulePendingStateUpdates();
  auto* monitor = sw_->getResolvedNexthopMonitor();
  EXPECT_TRUE(monitor->probesScheduled());
}

TEST_F(ResolvedNexthopMonitorTest, RemoveResolvedRoutes) {
  updateState(
      "resolved route added", [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{
            UnresolvedNextHop(folly::IPAddressV4("10.0.0.22"), 1),
            UnresolvedNextHop(folly::IPAddressV4("10.0.55.22"), 1)};
        return addRoute(state, kPrefixV4, nhops);
      });

  updateState(
      "resolved route removed", [=](const std::shared_ptr<SwitchState>& state) {
        return delRoute(state, kPrefixV4);
      });
  schedulePendingStateUpdates();
  auto* monitor = sw_->getResolvedNexthopMonitor();
  EXPECT_TRUE(monitor->probesScheduled());
}

TEST_F(ResolvedNexthopMonitorTest, ChangeUnresolvedRoutes) {
  updateState(
      "unresolved route added", [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{UnresolvedNextHop(kNexthopsV4[0], 1),
                              UnresolvedNextHop(kNexthopsV4[1], 1)};
        return addRoute(state, kPrefixV4, nhops);
      });

  updateState(
      "unresolved route change",
      [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{UnresolvedNextHop(kNexthopsV4[0], 1)};
        return addRoute(state, kPrefixV4, nhops);
      });

  schedulePendingStateUpdates();
  auto* monitor = sw_->getResolvedNexthopMonitor();
  EXPECT_FALSE(monitor->probesScheduled());
}

TEST_F(ResolvedNexthopMonitorTest, ProbeAddRemoveAdd) {
  updateState(
      "resolved route added", [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{
            ResolvedNextHop(folly::IPAddressV6("fe80::22"), InterfaceID(1), 1),
            ResolvedNextHop(
                folly::IPAddressV6("fe80:55::22"), InterfaceID(55), 1)};
        return addRoute(state, kPrefixV6, nhops);
      });
  schedulePendingStateUpdates();
  auto* scheduler = sw_->getResolvedNexthopProbeScheduler();
  auto resolvedNextHop2UseCount = scheduler->resolvedNextHop2UseCount();
  auto resolvedNextHop2Probes = scheduler->resolvedNextHop2Probes();

  // weight is ignored in next hop tracking
  std::vector<ResolvedNextHop> keys{
      ResolvedNextHop(folly::IPAddressV6("fe80::22"), InterfaceID(1), 0),
      ResolvedNextHop(folly::IPAddressV6("fe80:55::22"), InterfaceID(55), 0)};

  for (auto key : keys) {
    ASSERT_NE(
        resolvedNextHop2UseCount.find(key), resolvedNextHop2UseCount.end());
    EXPECT_EQ(resolvedNextHop2UseCount[key], 1);
    EXPECT_NE(resolvedNextHop2Probes.find(key), resolvedNextHop2Probes.end());
  }

  // removed next hop
  updateState(
      "resolved route change", [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{
            ResolvedNextHop(folly::IPAddressV6("fe80::22"), InterfaceID(1), 1)};
        return addRoute(state, kPrefixV6, nhops);
      });
  schedulePendingStateUpdates();
  resolvedNextHop2UseCount = scheduler->resolvedNextHop2UseCount();
  resolvedNextHop2Probes = scheduler->resolvedNextHop2Probes();

  for (auto key : keys) {
    if (key.intfID().value() == InterfaceID(1)) {
      ASSERT_NE(
          resolvedNextHop2UseCount.find(key), resolvedNextHop2UseCount.end());
      EXPECT_EQ(resolvedNextHop2UseCount[key], 1);
      EXPECT_NE(resolvedNextHop2Probes.find(key), resolvedNextHop2Probes.end());
    } else {
      EXPECT_EQ(
          resolvedNextHop2UseCount.find(key), resolvedNextHop2UseCount.end());
      EXPECT_EQ(resolvedNextHop2Probes.find(key), resolvedNextHop2Probes.end());
    }
  }

  // add next hop
  updateState(
      "resolved route added", [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{
            ResolvedNextHop(folly::IPAddressV6("fe80::22"), InterfaceID(1), 1),
            ResolvedNextHop(
                folly::IPAddressV6("fe80:55::22"), InterfaceID(55), 1)};
        return addRoute(state, kPrefixV6, nhops);
      });
  schedulePendingStateUpdates();
  resolvedNextHop2UseCount = scheduler->resolvedNextHop2UseCount();
  resolvedNextHop2Probes = scheduler->resolvedNextHop2Probes();

  for (auto key : keys) {
    ASSERT_NE(
        resolvedNextHop2UseCount.find(key), resolvedNextHop2UseCount.end());
    EXPECT_EQ(resolvedNextHop2UseCount[key], 1);
    EXPECT_NE(resolvedNextHop2Probes.find(key), resolvedNextHop2Probes.end());
  }
}

TEST_F(ResolvedNexthopMonitorTest, RouteSharingProbeTwoUpdates) {
  updateState(
      "resolved route added 1", [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{
            ResolvedNextHop(folly::IPAddressV6("fe80::22"), InterfaceID(1), 1),
            ResolvedNextHop(
                folly::IPAddressV6("fe80:55::22"), InterfaceID(55), 1)};
        return addRoute(state, kPrefixV6, nhops);
      });

  updateState(
      "resolved route added 2", [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{
            ResolvedNextHop(folly::IPAddressV6("fe80::22"), InterfaceID(1), 1),
            ResolvedNextHop(
                folly::IPAddressV6("fe80:55::22"), InterfaceID(55), 1)};
        return addRoute(
            state, RoutePrefixV6{IPAddressV6("10:101::"), 64}, nhops);
      });

  schedulePendingStateUpdates();
  auto* scheduler = sw_->getResolvedNexthopProbeScheduler();
  auto resolvedNextHop2UseCount = scheduler->resolvedNextHop2UseCount();
  auto resolvedNextHop2Probes = scheduler->resolvedNextHop2Probes();

  // weight is ignored in next hop tracking
  std::vector<ResolvedNextHop> keys{
      ResolvedNextHop(folly::IPAddressV6("fe80::22"), InterfaceID(1), 0),
      ResolvedNextHop(folly::IPAddressV6("fe80:55::22"), InterfaceID(55), 0)};

  for (auto key : keys) {
    ASSERT_NE(
        resolvedNextHop2UseCount.find(key), resolvedNextHop2UseCount.end());
    EXPECT_EQ(resolvedNextHop2UseCount[key], 2); // 2 routes refer these probes
    EXPECT_NE(resolvedNextHop2Probes.find(key), resolvedNextHop2Probes.end());
  }

  // removed route
  updateState(
      "resolved route change", [=](const std::shared_ptr<SwitchState>& state) {
        return delRoute(state, RoutePrefixV6{IPAddressV6("10:101::"), 64});
      });
  schedulePendingStateUpdates();
  resolvedNextHop2UseCount = scheduler->resolvedNextHop2UseCount();
  resolvedNextHop2Probes = scheduler->resolvedNextHop2Probes();

  for (auto key : keys) {
    ASSERT_NE(
        resolvedNextHop2UseCount.find(key), resolvedNextHop2UseCount.end());
    EXPECT_EQ(resolvedNextHop2UseCount[key], 1); // one route refers probe
    EXPECT_NE(resolvedNextHop2Probes.find(key), resolvedNextHop2Probes.end());
  }

  // remove another route
  updateState(
      "resolved route change", [=](const std::shared_ptr<SwitchState>& state) {
        return delRoute(state, kPrefixV6);
      });
  schedulePendingStateUpdates();
  resolvedNextHop2UseCount = scheduler->resolvedNextHop2UseCount();
  resolvedNextHop2Probes = scheduler->resolvedNextHop2Probes();

  for (auto key : keys) {
    // no probe exist
    ASSERT_EQ(
        resolvedNextHop2UseCount.find(key), resolvedNextHop2UseCount.end());
    ASSERT_EQ(resolvedNextHop2Probes.find(key), resolvedNextHop2Probes.end());
  }
}

TEST_F(ResolvedNexthopMonitorTest, RouteSharingProbeOneUpdate) {
  updateState(
      "resolved route added 1", [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{
            ResolvedNextHop(folly::IPAddressV6("fe80::22"), InterfaceID(1), 1),
            ResolvedNextHop(
                folly::IPAddressV6("fe80:55::22"), InterfaceID(55), 1)};
        auto newState = addRoute(state, kPrefixV6, nhops);
        return addRoute(
            newState, RoutePrefixV6{IPAddressV6("10:101::"), 64}, nhops);
      });

  schedulePendingStateUpdates();
  auto* scheduler = sw_->getResolvedNexthopProbeScheduler();
  auto resolvedNextHop2UseCount = scheduler->resolvedNextHop2UseCount();
  auto resolvedNextHop2Probes = scheduler->resolvedNextHop2Probes();

  // weight is ignored in next hop tracking
  std::vector<ResolvedNextHop> keys{
      ResolvedNextHop(folly::IPAddressV6("fe80::22"), InterfaceID(1), 0),
      ResolvedNextHop(folly::IPAddressV6("fe80:55::22"), InterfaceID(55), 0)};

  for (auto key : keys) {
    ASSERT_NE(
        resolvedNextHop2UseCount.find(key), resolvedNextHop2UseCount.end());
    EXPECT_EQ(resolvedNextHop2UseCount[key], 2);
    EXPECT_NE(resolvedNextHop2Probes.find(key), resolvedNextHop2Probes.end());
  }

  // removed route
  updateState(
      "resolved route change", [=](const std::shared_ptr<SwitchState>& state) {
        return delRoute(state, RoutePrefixV6{IPAddressV6("10:101::"), 64});
      });
  schedulePendingStateUpdates();
  resolvedNextHop2UseCount = scheduler->resolvedNextHop2UseCount();
  resolvedNextHop2Probes = scheduler->resolvedNextHop2Probes();

  for (auto key : keys) {
    ASSERT_NE(
        resolvedNextHop2UseCount.find(key), resolvedNextHop2UseCount.end());
    EXPECT_EQ(resolvedNextHop2UseCount[key], 1);
    EXPECT_NE(resolvedNextHop2Probes.find(key), resolvedNextHop2Probes.end());
  }

  // remove another route
  updateState(
      "resolved route change", [=](const std::shared_ptr<SwitchState>& state) {
        return delRoute(state, kPrefixV6);
      });
  schedulePendingStateUpdates();
  resolvedNextHop2UseCount = scheduler->resolvedNextHop2UseCount();
  resolvedNextHop2Probes = scheduler->resolvedNextHop2Probes();

  for (auto key : keys) {
    // no probe exist
    ASSERT_EQ(
        resolvedNextHop2UseCount.find(key), resolvedNextHop2UseCount.end());
    ASSERT_EQ(resolvedNextHop2Probes.find(key), resolvedNextHop2Probes.end());
  }
}

TEST_F(ResolvedNexthopMonitorTest, ProbeTriggeredV4) {
  auto arpTable =
      sw_->getState()->getVlans()->getVlan(VlanID(1))->getArpTable();
  EXPECT_EQ(arpTable->getEntryIf(folly::IPAddressV4("10.0.0.22")), nullptr);

  EXPECT_SWITCHED_PKT(sw_, "ARP request", [](const TxPacket* pkt) {
    const auto* buf = pkt->buf();
    EXPECT_EQ(68, buf->computeChainDataLength());
    folly::io::Cursor cursor(buf);
    EXPECT_EQ(
        PktUtil::readMac(&cursor), folly::MacAddress("ff:ff:ff:ff:ff:ff"));
    EXPECT_EQ(
        PktUtil::readMac(&cursor), folly::MacAddress("00:02:00:00:00:01"));
    EXPECT_EQ(cursor.readBE<uint16_t>(), 0x8100); // tagged frame
    EXPECT_EQ(cursor.readBE<uint16_t>(), 0x0001); // vlan tag
    EXPECT_EQ(cursor.readBE<uint16_t>(), 0x0806); // arp proto
    EXPECT_EQ(cursor.readBE<uint16_t>(), 0x0001); // ethernet
    EXPECT_EQ(cursor.readBE<uint16_t>(), 0x0800); // ipv4
    EXPECT_EQ(cursor.readBE<uint16_t>(), 0x0604); // hal, pal
    EXPECT_EQ(cursor.readBE<uint16_t>(), 0x0001); // arp req
    EXPECT_EQ(
        PktUtil::readMac(&cursor),
        folly::MacAddress("00:02:00:00:00:01")); // sender mac
    EXPECT_EQ(
        PktUtil::readIPv4(&cursor),
        folly::IPAddressV4("10.0.0.1")); // sender ip
    EXPECT_EQ(PktUtil::readMac(&cursor), folly::MacAddress::ZERO); // target mac
    EXPECT_EQ(
        PktUtil::readIPv4(&cursor),
        folly::IPAddressV4("10.0.0.22")); // target ip
  });
  updateState(
      "resolved route added", [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{
            UnresolvedNextHop(folly::IPAddressV4("10.0.0.22"), 1)};
        return addRoute(state, kPrefixV4, nhops);
      });
  schedulePendingStateUpdates();
  auto evb = sw_->getBackgroundEvb();
  evb->runInEventBaseThreadAndWait([]() {}); // let probe scheduler finish

  schedulePendingStateUpdates();
  // pending entry must be created
  arpTable = sw_->getState()->getVlans()->getVlan(VlanID(1))->getArpTable();
  auto entry = arpTable->getEntryIf(folly::IPAddressV4("10.0.0.22"));
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);
}

TEST_F(ResolvedNexthopMonitorTest, ProbeTriggeredOnEntryRemoveV4) {
  updateState("add neighbor", [](const std::shared_ptr<SwitchState> state) {
    auto newState = state->clone();
    auto vlan = state->getVlans()->getVlan(VlanID(1));
    auto arpTable = vlan->getArpTable()->modify(VlanID(1), &newState);
    arpTable->addEntry(
        folly::IPAddressV4("10.0.0.22"),
        folly::MacAddress("02:09:00:00:00:22"),
        PortDescriptor(PortID(1)),
        InterfaceID(1),
        NeighborState::REACHABLE);
    return newState;
  });

  auto entry = sw_->getState()
                   ->getVlans()
                   ->getVlan(VlanID(1))
                   ->getArpTable()
                   ->getEntryIf(folly::IPAddressV4("10.0.0.22"));
  ASSERT_NE(entry, nullptr);

  updateState(
      "resolved route added", [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{
            UnresolvedNextHop(folly::IPAddressV4("10.0.0.22"), 1)};
        return addRoute(state, kPrefixV4, nhops);
      });

  entry = sw_->getState()
              ->getVlans()
              ->getVlan(VlanID(1))
              ->getArpTable()
              ->getEntryIf(folly::IPAddressV4("10.0.0.22"));
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), false); // no probe

  EXPECT_SWITCHED_PKT(sw_, "ARP request", [](const TxPacket* pkt) {
    const auto* buf = pkt->buf();
    EXPECT_EQ(68, buf->computeChainDataLength());
    folly::io::Cursor cursor(buf);
    EXPECT_EQ(
        PktUtil::readMac(&cursor), folly::MacAddress("ff:ff:ff:ff:ff:ff"));
    EXPECT_EQ(
        PktUtil::readMac(&cursor), folly::MacAddress("00:02:00:00:00:01"));
    EXPECT_EQ(cursor.readBE<uint16_t>(), 0x8100); // tagged frame
    EXPECT_EQ(cursor.readBE<uint16_t>(), 0x0001); // vlan tag
    EXPECT_EQ(cursor.readBE<uint16_t>(), 0x0806); // arp proto
    EXPECT_EQ(cursor.readBE<uint16_t>(), 0x0001); // ethernet
    EXPECT_EQ(cursor.readBE<uint16_t>(), 0x0800); // ipv4
    EXPECT_EQ(cursor.readBE<uint16_t>(), 0x0604); // hal, pal
    EXPECT_EQ(cursor.readBE<uint16_t>(), 0x0001); // arp req
    EXPECT_EQ(
        PktUtil::readMac(&cursor),
        folly::MacAddress("00:02:00:00:00:01")); // sender mac
    EXPECT_EQ(
        PktUtil::readIPv4(&cursor),
        folly::IPAddressV4("10.0.0.1")); // sender ip
    EXPECT_EQ(PktUtil::readMac(&cursor), folly::MacAddress::ZERO); // target mac
    EXPECT_EQ(
        PktUtil::readIPv4(&cursor),
        folly::IPAddressV4("10.0.0.22")); // target ip
  });

  updateState("remove neighbor", [](const std::shared_ptr<SwitchState> state) {
    auto newState = state->clone();
    auto vlan = state->getVlans()->getVlan(VlanID(1));
    auto arpTable = vlan->getArpTable()->modify(VlanID(1), &newState);
    arpTable->removeEntry(folly::IPAddressV4("10.0.0.22"));
    return newState;
  });

  schedulePendingStateUpdates();
  auto evb = sw_->getBackgroundEvb();
  evb->runInEventBaseThreadAndWait([]() {}); // let probe scheduler finish
  schedulePendingStateUpdates();
  // pending entry must be created
  entry = sw_->getState()
              ->getVlans()
              ->getVlan(VlanID(1))
              ->getArpTable()
              ->getEntryIf(folly::IPAddressV4("10.0.0.22"));
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);
}

TEST_F(ResolvedNexthopMonitorTest, ProbeTriggeredV6) {
  auto ndpTable =
      sw_->getState()->getVlans()->getVlan(VlanID(1))->getNdpTable();
  EXPECT_EQ(
      ndpTable->getEntryIf(folly::IPAddressV6("2401:db00:2110:3001::22")),
      nullptr);

  EXPECT_SWITCHED_PKT(sw_, "NDP request", [](const TxPacket* pkt) {
    folly::io::Cursor cursor(pkt->buf());

    EXPECT_EQ(
        PktUtil::readMac(&cursor),
        folly::MacAddress("33:33:ff:00:00:22")); // dest mac
    EXPECT_EQ(
        PktUtil::readMac(&cursor),
        folly::MacAddress("00:02:00:00:00:01")); // src mac
    EXPECT_EQ(cursor.readBE<uint16_t>(), 0x8100); // tagged frame
    EXPECT_EQ(cursor.readBE<uint16_t>(), 0x0001); // vlan tag
    EXPECT_EQ(cursor.readBE<uint16_t>(), 0x86dd); // ipv6 proto

    IPv6Hdr v6Hdr(cursor); // v6 hdr
    EXPECT_EQ(
        v6Hdr.nextHeader, static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_ICMP));
    EXPECT_EQ(
        v6Hdr.srcAddr,
        folly::IPAddressV6("fe80:0000:0000:0000:0202:00ff:fe00:0001"));
    EXPECT_EQ(
        v6Hdr.dstAddr,
        folly::IPAddressV6("ff02:0000:0000:0000:0000:0001:ff00:0022"));

    ICMPHdr icmp6Hdr(cursor); // v6 icmp hdr
    EXPECT_EQ(
        icmp6Hdr.type,
        static_cast<uint8_t>(
            ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION));
    EXPECT_EQ(icmp6Hdr.code, 0);
    cursor.read<uint32_t>(); // reserved
    EXPECT_EQ(
        PktUtil::readIPv6(&cursor),
        folly::IPAddressV6("2401:db00:2110:3001::22"));
    EXPECT_EQ(1, cursor.read<uint8_t>()); // source link layer address option
    EXPECT_EQ(1, cursor.read<uint8_t>()); // option length
    EXPECT_EQ(
        PktUtil::readMac(&cursor), folly::MacAddress("00:02:00:00:00:01"));
  });
  updateState(
      "resolved route added", [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{UnresolvedNextHop(
            folly::IPAddressV6("2401:db00:2110:3001::22"), 1)};
        return addRoute(state, kPrefixV6, nhops);
      });

  schedulePendingStateUpdates();
  auto evb = sw_->getBackgroundEvb();
  evb->runInEventBaseThreadAndWait([]() {});
  schedulePendingStateUpdates();
  // pending entry must be created
  ndpTable = sw_->getState()->getVlans()->getVlan(VlanID(1))->getNdpTable();
  auto entry =
      ndpTable->getEntryIf(folly::IPAddressV6("2401:db00:2110:3001::22"));
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);
}

TEST_F(ResolvedNexthopMonitorTest, ProbeTriggeredOnEntryRemoveV6) {
  updateState("add neighbor", [](const std::shared_ptr<SwitchState> state) {
    auto newState = state->clone();
    auto vlan = state->getVlans()->getVlan(VlanID(1));
    auto ndpTable = vlan->getNdpTable()->modify(VlanID(1), &newState);
    ndpTable->addEntry(
        folly::IPAddressV6("2401:db00:2110:3001::22"),
        folly::MacAddress("02:09:00:00:00:22"),
        PortDescriptor(PortID(1)),
        InterfaceID(1),
        NeighborState::REACHABLE);
    return newState;
  });

  auto entry = sw_->getState()
                   ->getVlans()
                   ->getVlan(VlanID(1))
                   ->getNdpTable()
                   ->getEntryIf(folly::IPAddressV6("2401:db00:2110:3001::22"));
  ASSERT_NE(entry, nullptr);

  updateState(
      "resolved route added", [=](const std::shared_ptr<SwitchState>& state) {
        RouteNextHopSet nhops{UnresolvedNextHop(
            folly::IPAddressV6("2401:db00:2110:3001::22"), 1)};
        return addRoute(state, kPrefixV6, nhops);
      });

  entry = sw_->getState()
              ->getVlans()
              ->getVlan(VlanID(1))
              ->getNdpTable()
              ->getEntryIf(folly::IPAddressV6("2401:db00:2110:3001::22"));
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), false); // no probe

  EXPECT_SWITCHED_PKT(sw_, "NDP request", [](const TxPacket* pkt) {
    folly::io::Cursor cursor(pkt->buf());

    EXPECT_EQ(
        PktUtil::readMac(&cursor),
        folly::MacAddress("33:33:ff:00:00:22")); // dest mac
    EXPECT_EQ(
        PktUtil::readMac(&cursor),
        folly::MacAddress("00:02:00:00:00:01")); // src mac
    EXPECT_EQ(cursor.readBE<uint16_t>(), 0x8100); // tagged frame
    EXPECT_EQ(cursor.readBE<uint16_t>(), 0x0001); // vlan tag
    EXPECT_EQ(cursor.readBE<uint16_t>(), 0x86dd); // ipv6 proto

    IPv6Hdr v6Hdr(cursor); // v6 hdr
    EXPECT_EQ(
        v6Hdr.nextHeader, static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_ICMP));
    EXPECT_EQ(
        v6Hdr.srcAddr,
        folly::IPAddressV6("fe80:0000:0000:0000:0202:00ff:fe00:0001"));
    EXPECT_EQ(
        v6Hdr.dstAddr,
        folly::IPAddressV6("ff02:0000:0000:0000:0000:0001:ff00:0022"));

    ICMPHdr icmp6Hdr(cursor); // v6 icmp hdr
    EXPECT_EQ(
        icmp6Hdr.type,
        static_cast<uint8_t>(
            ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION));
    EXPECT_EQ(icmp6Hdr.code, 0);
    cursor.read<uint32_t>(); // reserved
    EXPECT_EQ(
        PktUtil::readIPv6(&cursor),
        folly::IPAddressV6("2401:db00:2110:3001::22"));
    EXPECT_EQ(1, cursor.read<uint8_t>()); // source link layer address option
    EXPECT_EQ(1, cursor.read<uint8_t>()); // option length
    EXPECT_EQ(
        PktUtil::readMac(&cursor), folly::MacAddress("00:02:00:00:00:01"));
  });

  updateState("remove neighbor", [](const std::shared_ptr<SwitchState> state) {
    auto newState = state->clone();
    auto vlan = state->getVlans()->getVlan(VlanID(1));
    auto ndpTable = vlan->getNdpTable()->modify(VlanID(1), &newState);
    ndpTable->removeEntry(folly::IPAddressV6("2401:db00:2110:3001::22"));
    return newState;
  });

  schedulePendingStateUpdates();
  auto evb = sw_->getBackgroundEvb();
  evb->runInEventBaseThreadAndWait([]() {}); // let probe scheduler finish
  schedulePendingStateUpdates();
  // pending entry must be created
  entry = sw_->getState()
              ->getVlans()
              ->getVlan(VlanID(1))
              ->getNdpTable()
              ->getEntryIf(folly::IPAddressV6("2401:db00:2110:3001::22"));
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);
}

} // namespace facebook::fboss
