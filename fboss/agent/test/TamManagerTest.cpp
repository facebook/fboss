/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/TamManager.h"

#include <array>
#include <map>
#include <optional>
#include <string>
#include <utility>

#include <folly/Function.h>
#include <folly/IPAddress.h>
#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/Range.h>
#include <gtest/gtest.h>

#include "fboss/agent/ArpHandler.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/packet/ICMPHdr.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/MirrorOnDropReport.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

namespace {
const std::string kReportName = "mod_report";
constexpr AdminDistance kDistance = AdminDistance::STATIC_ROUTE;
const PortID kMirrorPortId{0};

template <typename AddrT>
struct TamManagerTestParams {
  AddrT collectorIp;
  folly::IPAddress localSrcIp;
  std::array<AddrT, 2> neighborIPs;
  std::array<folly::MacAddress, 2> neighborMACs;
  std::array<PortID, 2> neighborPorts;
  std::array<InterfaceID, 2> interfaces;
  RoutePrefix<AddrT> longerPrefix;
  RoutePrefix<AddrT> shorterPrefix;

  TamManagerTestParams(
      const AddrT& collectorIp,
      const folly::IPAddress& localSrcIp,
      std::array<AddrT, 2>&& neighborIPs,
      std::array<folly::MacAddress, 2>&& neighborMACs,
      std::array<PortID, 2>&& neighborPorts,
      std::array<InterfaceID, 2>&& interfaces,
      const RoutePrefix<AddrT>& longerPrefix,
      const RoutePrefix<AddrT>& shorterPrefix)
      : collectorIp(collectorIp),
        localSrcIp(localSrcIp),
        neighborIPs(std::move(neighborIPs)),
        neighborMACs(std::move(neighborMACs)),
        neighborPorts(std::move(neighborPorts)),
        interfaces(std::move(interfaces)),
        longerPrefix(longerPrefix),
        shorterPrefix(shorterPrefix) {}

  const UnresolvedNextHop nextHop(int neighborIndex) const {
    return UnresolvedNextHop(neighborIPs[neighborIndex % 2], NextHopWeight(80));
  }
};

template <typename AddrT>
TamManagerTestParams<AddrT> getParams() {
  if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
    return TamManagerTestParams<AddrT>(
        folly::IPAddressV4("10.0.10.101"),
        folly::IPAddress("10.0.0.1"),
        {folly::IPAddressV4("10.0.0.111"), folly::IPAddressV4("10.0.55.111")},
        {folly::MacAddress("10:0:0:0:1:11"),
         folly::MacAddress("10:0:0:55:1:11")},
        {PortID(6), PortID(7)},
        {InterfaceID(1), InterfaceID(55)},
        {folly::IPAddressV4("10.0.10.100"), 31},
        {folly::IPAddressV4("10.0.0.0"), 16});
  } else {
    return TamManagerTestParams<AddrT>(
        folly::IPAddressV6("2401:db00:2110:10::1001"),
        folly::IPAddress("2401:db00:2110:3001::0001"),
        {folly::IPAddressV6("2401:db00:2110:3001::0111"),
         folly::IPAddressV6("2401:db00:2110:3055::0111")},
        {folly::MacAddress("10:0:0:0:1:11"),
         folly::MacAddress("10:0:0:55:1:11")},
        {PortID(6), PortID(7)},
        {InterfaceID(1), InterfaceID(55)},
        {folly::IPAddressV6("2401:db00:2110:10::1000"), 127},
        {folly::IPAddressV6("2401:db00:2110:10::0000"), 64});
  }
}
} // namespace

using TestTypes = ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;

struct TestTypeNames {
  template <typename T>
  static std::string GetName(int) {
    if constexpr (std::is_same_v<T, folly::IPAddressV4>) {
      return "V4";
    } else {
      return "V6";
    }
  }
};

template <typename AddrType>
class TamManagerTest : public ::testing::Test {
 public:
  using Func = folly::Function<void()>;
  using StateUpdateFn = SwSwitch::StateUpdateFn;
  using AddrT = AddrType;

  void SetUp() override {
    cfg::SwitchConfig config = testConfigA();
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
  }

  TamManagerTestParams<AddrT> getParamsHelper() {
    return getParams<AddrT>();
  }

  void verifyStateUpdate(Func func) {
    runInUpdateEventBaseAndWait(std::move(func));
  }

  void TearDown() override {
    schedulePendingTestStateUpdates();
  }

  void updateState(folly::StringPiece name, StateUpdateFn func) {
    sw_->updateStateBlocking(name, std::move(func));
  }

  std::shared_ptr<SwitchState> addMirrorOnDropReport(
      const std::shared_ptr<SwitchState>& state,
      const std::string& name,
      const folly::IPAddress& localSrcIp,
      const AddrT& collectorIp,
      PortID mirrorPortId = PortID(0)) {
    std::shared_ptr<SwitchState> newState = state->clone();

    std::shared_ptr<MirrorOnDropReport> report = std::make_shared<
        MirrorOnDropReport>(
        name,
        mirrorPortId,
        localSrcIp,
        static_cast<int16_t>(12345), // localSrcPort
        folly::IPAddress(collectorIp), // collectorIp
        static_cast<int16_t>(6343), // collectorPort
        static_cast<int16_t>(1500), // mtu
        static_cast<int16_t>(128), // truncateSize
        static_cast<uint8_t>(0), // dscp
        std::string("00:00:00:00:00:01"), // switchMac
        std::string("00:00:00:00:00:02"), // firstInterfaceMac
        std::map<int8_t, cfg::MirrorOnDropEventConfig>{}, // modEventToConfigMap
        std::map<
            cfg::MirrorOnDropAgingGroup,
            int32_t>{}); // agingGroupAgingIntervalUsecs

    newState->getMirrorOnDropReports()->modify(&newState)->addNode(
        report, sw_->getScopeResolver()->scope(report));
    return newState;
  }

  void addRoute(const RoutePrefix<AddrT>& prefix, RouteNextHopSet nexthops) {
    SwSwitchRouteUpdateWrapper updater = sw_->getRouteUpdater();
    updater.addRoute(
        RouterID(0),
        prefix.network(),
        prefix.mask(),
        ClientID(1000),
        RouteNextHopEntry(std::move(nexthops), kDistance));
    updater.program();
  }

  void delRoute(const RoutePrefix<AddrT>& prefix) {
    SwSwitchRouteUpdateWrapper updater = sw_->getRouteUpdater();
    updater.delRoute(
        RouterID(0), prefix.network(), prefix.mask(), ClientID(1000));
    updater.program();
  }

  void resolveNeighbor(
      folly::IPAddress ipAddress,
      folly::MacAddress macAddress,
      InterfaceID interfaceID,
      const PortID& portID,
      bool wait = true) {
    // Neighbor tables are always on interfaces
    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      sw_->getNeighborUpdater()->receivedArpMineForIntf(
          interfaceID,
          ipAddress.asV4(),
          macAddress,
          PortDescriptor(portID),
          ArpOpCode::ARP_OP_REPLY);
    } else {
      sw_->getNeighborUpdater()->receivedNdpMineForIntf(
          interfaceID,
          ipAddress.asV6(),
          macAddress,
          PortDescriptor(portID),
          ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION,
          0);
    }
    if (wait) {
      sw_->getNeighborUpdater()->waitForPendingUpdates();
      waitForBackgroundThread(sw_);
      waitForStateUpdates(sw_);
      sw_->getNeighborUpdater()->waitForPendingUpdates();
      waitForStateUpdates(sw_);
    }
  }

 protected:
  void runInUpdateEventBaseAndWait(Func func) {
    sw_->getUpdateEvb()->runInFbossEventBaseThreadAndWait(std::move(func));
  }

  void schedulePendingTestStateUpdates() {
    runInUpdateEventBaseAndWait([]() {});
  }

  std::unique_ptr<HwTestHandle> handle_;
  SwSwitch* sw_{};
};

TYPED_TEST_SUITE(TamManagerTest, TestTypes, TestTypeNames);

TYPED_TEST(TamManagerTest, ReportNotResolvedWithoutRoutes) {
  TamManagerTestParams<typename TestFixture::AddrT> params =
      this->getParamsHelper();

  this->updateState(
      "ReportNotResolvedWithoutRoutes",
      [=, this](const std::shared_ptr<SwitchState>& state) {
        return this->addMirrorOnDropReport(
            state, kReportName, params.localSrcIp, params.collectorIp);
      });

  this->verifyStateUpdate([=, this]() {
    std::shared_ptr<MirrorOnDropReport> report =
        this->sw_->getState()->getMirrorOnDropReports()->getNodeIf(kReportName);
    EXPECT_NE(report, nullptr);
    EXPECT_FALSE(report->isResolved());
  });
}

TYPED_TEST(TamManagerTest, ReportNotResolvedWithoutNeighbor) {
  TamManagerTestParams<typename TestFixture::AddrT> params =
      this->getParamsHelper();

  this->updateState(
      "ReportNotResolvedWithoutNeighbor",
      [=, this](const std::shared_ptr<SwitchState>& state) {
        return this->addMirrorOnDropReport(
            state, kReportName, params.localSrcIp, params.collectorIp);
      });

  this->addRoute(params.longerPrefix, {params.nextHop(0)});

  this->verifyStateUpdate([=, this]() {
    std::shared_ptr<MirrorOnDropReport> report =
        this->sw_->getState()->getMirrorOnDropReports()->getNodeIf(kReportName);
    EXPECT_NE(report, nullptr);
    EXPECT_FALSE(report->isResolved());
  });
}

TYPED_TEST(TamManagerTest, ResolveReportWithRouteAndNeighbor) {
  TamManagerTestParams<typename TestFixture::AddrT> params =
      this->getParamsHelper();

  this->updateState(
      "ResolveReportWithRouteAndNeighbor: addReport",
      [=, this](const std::shared_ptr<SwitchState>& state) {
        return this->addMirrorOnDropReport(
            state, kReportName, params.localSrcIp, params.collectorIp);
      });

  this->addRoute(params.longerPrefix, {params.nextHop(0)});

  this->resolveNeighbor(
      folly::IPAddress(params.neighborIPs[0]),
      params.neighborMACs[0],
      params.interfaces[0],
      params.neighborPorts[0]);

  this->verifyStateUpdate([=, this]() {
    std::shared_ptr<MirrorOnDropReport> report =
        this->sw_->getState()->getMirrorOnDropReports()->getNodeIf(kReportName);
    EXPECT_NE(report, nullptr);
    EXPECT_TRUE(report->isResolved());
    EXPECT_EQ(
        report->getResolvedCollectorMac().value(), params.neighborMACs[0]);
    std::optional<cfg::PortDescriptor> resolvedPort =
        report->getResolvedEgressPort();
    EXPECT_TRUE(resolvedPort.has_value());
    EXPECT_EQ(PortID(*resolvedPort->portId()), params.neighborPorts[0]);
  });
}

TYPED_TEST(TamManagerTest, UpdateReportOnRouteChange) {
  TamManagerTestParams<typename TestFixture::AddrT> params =
      this->getParamsHelper();

  this->updateState(
      "UpdateReportOnRouteChange: addReport",
      [=, this](const std::shared_ptr<SwitchState>& state) {
        return this->addMirrorOnDropReport(
            state, kReportName, params.localSrcIp, params.collectorIp);
      });

  this->addRoute(params.longerPrefix, {params.nextHop(0)});
  this->addRoute(params.shorterPrefix, {params.nextHop(1)});

  this->resolveNeighbor(
      folly::IPAddress(params.neighborIPs[0]),
      params.neighborMACs[0],
      params.interfaces[0],
      params.neighborPorts[0]);

  this->resolveNeighbor(
      folly::IPAddress(params.neighborIPs[1]),
      params.neighborMACs[1],
      params.interfaces[1],
      params.neighborPorts[1]);

  this->verifyStateUpdate([=, this]() {
    std::shared_ptr<MirrorOnDropReport> report =
        this->sw_->getState()->getMirrorOnDropReports()->getNodeIf(kReportName);
    EXPECT_NE(report, nullptr);
    EXPECT_TRUE(report->isResolved());
    EXPECT_EQ(
        report->getResolvedCollectorMac().value(), params.neighborMACs[0]);
    std::optional<cfg::PortDescriptor> resolvedPort =
        report->getResolvedEgressPort();
    EXPECT_TRUE(resolvedPort.has_value());
    EXPECT_EQ(PortID(*resolvedPort->portId()), params.neighborPorts[0]);
  });

  this->delRoute(params.longerPrefix);

  this->verifyStateUpdate([=, this]() {
    std::shared_ptr<MirrorOnDropReport> report =
        this->sw_->getState()->getMirrorOnDropReports()->getNodeIf(kReportName);
    EXPECT_NE(report, nullptr);
    EXPECT_TRUE(report->isResolved());
    EXPECT_EQ(
        report->getResolvedCollectorMac().value(), params.neighborMACs[1]);
    std::optional<cfg::PortDescriptor> resolvedPort =
        report->getResolvedEgressPort();
    EXPECT_TRUE(resolvedPort.has_value());
    EXPECT_EQ(PortID(*resolvedPort->portId()), params.neighborPorts[1]);
  });
}

TYPED_TEST(TamManagerTest, ResolveReportOnNeighborAdd) {
  TamManagerTestParams<typename TestFixture::AddrT> params =
      this->getParamsHelper();

  this->updateState(
      "ResolveReportOnNeighborAdd: addReport",
      [=, this](const std::shared_ptr<SwitchState>& state) {
        return this->addMirrorOnDropReport(
            state, kReportName, params.localSrcIp, params.collectorIp);
      });

  this->addRoute(params.longerPrefix, {params.nextHop(0)});

  this->verifyStateUpdate([=, this]() {
    std::shared_ptr<MirrorOnDropReport> report =
        this->sw_->getState()->getMirrorOnDropReports()->getNodeIf(kReportName);
    EXPECT_NE(report, nullptr);
    EXPECT_FALSE(report->isResolved());
  });

  this->resolveNeighbor(
      folly::IPAddress(params.neighborIPs[0]),
      params.neighborMACs[0],
      params.interfaces[0],
      params.neighborPorts[0]);

  this->verifyStateUpdate([=, this]() {
    std::shared_ptr<MirrorOnDropReport> report =
        this->sw_->getState()->getMirrorOnDropReports()->getNodeIf(kReportName);
    EXPECT_NE(report, nullptr);
    EXPECT_TRUE(report->isResolved());
    EXPECT_EQ(
        report->getResolvedCollectorMac().value(), params.neighborMACs[0]);
    std::optional<cfg::PortDescriptor> resolvedPort =
        report->getResolvedEgressPort();
    EXPECT_TRUE(resolvedPort.has_value());
    EXPECT_EQ(PortID(*resolvedPort->portId()), params.neighborPorts[0]);
  });
}

TYPED_TEST(TamManagerTest, ResolveWithDirectlyConnectedRoute) {
  TamManagerTestParams<typename TestFixture::AddrT> params =
      this->getParamsHelper();

  this->updateState(
      "ResolveWithDirectlyConnectedRoute: addReport",
      [=, this](const std::shared_ptr<SwitchState>& state) {
        return this->addMirrorOnDropReport(
            state, kReportName, params.localSrcIp, params.neighborIPs[0]);
      });

  this->resolveNeighbor(
      folly::IPAddress(params.neighborIPs[0]),
      params.neighborMACs[0],
      params.interfaces[0],
      params.neighborPorts[0]);

  this->verifyStateUpdate([=, this]() {
    std::shared_ptr<MirrorOnDropReport> report =
        this->sw_->getState()->getMirrorOnDropReports()->getNodeIf(kReportName);
    EXPECT_NE(report, nullptr);
    EXPECT_TRUE(report->isResolved());
    EXPECT_EQ(
        report->getResolvedCollectorMac().value(), params.neighborMACs[0]);
    std::optional<cfg::PortDescriptor> resolvedPort =
        report->getResolvedEgressPort();
    EXPECT_TRUE(resolvedPort.has_value());
    EXPECT_EQ(PortID(*resolvedPort->portId()), params.neighborPorts[0]);
  });
}

TYPED_TEST(TamManagerTest, ResolveReportOnAdd) {
  TamManagerTestParams<typename TestFixture::AddrT> params =
      this->getParamsHelper();

  // First, set up the route and resolve the neighbor
  this->addRoute(params.longerPrefix, {params.nextHop(0)});

  this->resolveNeighbor(
      folly::IPAddress(params.neighborIPs[0]),
      params.neighborMACs[0],
      params.interfaces[0],
      params.neighborPorts[0]);

  // Now add the report - it should be resolved immediately
  this->updateState(
      "ResolveReportOnAdd: addReport",
      [=, this](const std::shared_ptr<SwitchState>& state) {
        return this->addMirrorOnDropReport(
            state, kReportName, params.localSrcIp, params.collectorIp);
      });

  this->verifyStateUpdate([=, this]() {
    std::shared_ptr<MirrorOnDropReport> report =
        this->sw_->getState()->getMirrorOnDropReports()->getNodeIf(kReportName);
    EXPECT_NE(report, nullptr);
    EXPECT_TRUE(report->isResolved());
    EXPECT_EQ(
        report->getResolvedCollectorMac().value(), params.neighborMACs[0]);
    std::optional<cfg::PortDescriptor> resolvedPort =
        report->getResolvedEgressPort();
    EXPECT_TRUE(resolvedPort.has_value());
    EXPECT_EQ(PortID(*resolvedPort->portId()), params.neighborPorts[0]);
  });
}

TYPED_TEST(TamManagerTest, SkipResolutionWithStaticMirrorPort) {
  TamManagerTestParams<typename TestFixture::AddrT> params =
      this->getParamsHelper();

  this->updateState(
      "SkipResolutionWithStaticMirrorPort: addReport",
      [=, this](const std::shared_ptr<SwitchState>& state) {
        return this->addMirrorOnDropReport(
            state,
            kReportName,
            params.localSrcIp,
            params.collectorIp,
            PortID(5));
      });

  this->addRoute(params.longerPrefix, {params.nextHop(0)});

  this->resolveNeighbor(
      folly::IPAddress(params.neighborIPs[0]),
      params.neighborMACs[0],
      params.interfaces[0],
      params.neighborPorts[0]);

  this->verifyStateUpdate([=, this]() {
    std::shared_ptr<MirrorOnDropReport> report =
        this->sw_->getState()->getMirrorOnDropReports()->getNodeIf(kReportName);
    EXPECT_NE(report, nullptr);
    EXPECT_FALSE(report->isResolved());
  });
}

} // namespace facebook::fboss
