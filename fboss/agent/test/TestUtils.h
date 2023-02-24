/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <gmock/gmock.h>
#include <functional>
#include <memory>

#include <folly/Function.h>
#include <folly/MacAddress.h>
#include <folly/Range.h>
#include <folly/synchronization/Baton.h>
#include <optional>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/StateObserver.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/mock/MockHwSwitch.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/test/RouteDistributionGenerator.h"

namespace facebook::fboss {

class MockHwSwitch;
class MockPlatform;
class MockTunManager;
class Platform;
class RxPacket;
class SwitchState;
class SwSwitch;
class TxPacket;
class HwTestHandle;
class RoutingInformationBase;

namespace cfg {
class SwitchConfig;
}

template <cfg::SwitchType type>
struct SwitchTypeT {
  static constexpr auto switchType = type;
};

using SwitchTypes = ::testing::Types<
    SwitchTypeT<cfg::SwitchType::NPU>,
    SwitchTypeT<cfg::SwitchType::VOQ>,
    SwitchTypeT<cfg::SwitchType::FABRIC>>;

/*
 * In the non unit test code state passed to apply*Config is the state
 * returned from SwSwitch init, which is always published. However this
 * is not the case for unit test code where we are constructing config
 * by hand and applying it to a (often empty) initial state. Now in a rare
 * case if such a config had only route updates in it, since the previous
 * state was not published, we would apply these route updates inline
 * and now when we compare the old and new route tables to determine
 * if anything changed we would come up empty. The end result in such
 * a case would be to return a null new state falsely signifying no change.
 * To avoid this, provide convenience wrappers for unit test code to call.
 */
std::shared_ptr<SwitchState> publishAndApplyConfig(
    const std::shared_ptr<SwitchState>& state,
    const cfg::SwitchConfig* config,
    const Platform* platform,
    RoutingInformationBase* rib = nullptr);

/*
 * Create a SwSwitch for testing purposes, with the specified initial state.
 *
 * This will use a MockHwSwitch for the HwSwitch implementation, with the
 * specified initial state.  The returned SwSwitch will have already been
 * initialized.
 */
std::unique_ptr<HwTestHandle> createTestHandle(
    const std::shared_ptr<SwitchState>& = nullptr,
    SwitchFlags flags = SwitchFlags::DEFAULT);
std::unique_ptr<HwTestHandle> createTestHandle(
    cfg::SwitchConfig* cfg,
    SwitchFlags flags = SwitchFlags::DEFAULT);

std::unique_ptr<MockPlatform> createMockPlatform(
    cfg::SwitchType switchType = cfg::SwitchType::NPU,
    std::optional<int64_t> switchId = std::nullopt);
std::unique_ptr<SwSwitch> setupMockSwitchWithoutHW(
    std::unique_ptr<MockPlatform> platform,
    const std::shared_ptr<SwitchState>& state,
    SwitchFlags flags);

std::unique_ptr<SwSwitch> setupMockSwitchWithHW(
    std::unique_ptr<MockPlatform> platform,
    const std::shared_ptr<SwitchState>& state,
    SwitchFlags flags);
cfg::DsfNode makeDsfNodeCfg(
    int64_t switchId = 0,
    cfg::DsfNodeType type = cfg::DsfNodeType::INTERFACE_NODE);

cfg::SwitchConfig updateSwitchID(
    const cfg::SwitchConfig& origCfg,
    int64_t oldSwitchId,
    int64_t newSwitchId);

std::shared_ptr<SystemPort> makeSysPort(
    const std::optional<std::string>& qosPolicy,
    int64_t sysPortId = 1,
    int64_t switchId = 1);
/*
 * Get the MockHwSwitch from a SwSwitch.
 */
MockHwSwitch* getMockHw(SwSwitch* sw);
MockHwSwitch* getMockHw(std::unique_ptr<SwSwitch>& sw);

/*
 * Get the MockPlatform from a SwSwitch.
 */
MockPlatform* getMockPlatform(SwSwitch* sw);
MockPlatform* getMockPlatform(std::unique_ptr<SwSwitch>& sw);

/*
 * Wait until all pending StateUpdates have been applied. Take a
 * snapshot and return the state at the time all previous updates have
 * been processed.
 */
std::shared_ptr<SwitchState> waitForStateUpdates(SwSwitch* sw);

/*
 * Wait until all pending actions on the background thread
 * have been processed
 */
void waitForBackgroundThread(SwSwitch* sw);

/*
 * Wait until all pending actions on the neighbor cache thread
 * have been processed
 */
void waitForNeighborCacheThread(SwSwitch* sw);

/*
 * Wait until all currently queued lambdas on ribUpdateThread are
 * done
 */
void waitForRibUpdates(SwSwitch* sw);

/*
 * Update Blocked Neighbors list and wait for updates
 */
void updateBlockedNeighbor(
    SwSwitch* sw,
    const std::vector<std::pair<VlanID, folly::IPAddress>>& ipAddresses);

/*
 * Update Blocked MAC addr list and wait for updates
 */
void updateMacAddrsToBlock(
    SwSwitch* sw,
    const std::vector<std::pair<VlanID, folly::MacAddress>>& macAddrsToBlock);

/**
 * check the field value
 */
template <typename ExpectedType, typename ActualType>
void checkField(
    const ExpectedType& expected,
    const ActualType& actual,
    folly::StringPiece fieldName) {
  if (expected != actual) {
    throw FbossError(
        "expected ", fieldName, " to be ", expected, ", but found ", actual);
    return;
  }
}

/*
 * Create a SwitchState for testing.
 *
 * Profile A:
 *   - 20 ports, 2 VLANS, 2 interfaces
 *   - Ports 1-10 are in VLAN 1
 *   - Ports 11-20 are in VLAN 55
 *   - Interface 1:
 *     - VLAN 1
 *     - MAC 00:02:00:00:00:01
 *     - IPs:
 *       10.0.0.1/24
 *       192.168.0.1/24
 *       2401:db00:2110:3001::0001/64
 *   - Interface 55:
 *     - VLAN 55
 *     - MAC 00:02:00:00:00:55
 *     - IPs:
 *       10.0.55.1/24
 *       192.168.55.1/24
 *       2401:db00:2110:3055::0001/64
 */
std::shared_ptr<SwitchState> testStateA();
/*
 * Same as testStateA but with all ports
 * enabled and up
 */
std::shared_ptr<SwitchState> testStateAWithPortsUp();

/*
 * Same as testStateA but with AclLookupClass associated with every port.
 * (MH-NIC case queue-per-host configuration).
 */
std::shared_ptr<SwitchState> testStateAWithLookupClasses();

std::shared_ptr<SwitchState> testStateAWithoutIpv4VlanIntf(VlanID vlanId);

/*
 * Bring all ports up for a given input state
 */
std::shared_ptr<SwitchState> bringAllPortsUp(
    const std::shared_ptr<SwitchState>& in);

/*
 * Bring all ports down for a given input state
 */
std::shared_ptr<SwitchState> bringAllPortsDown(
    const std::shared_ptr<SwitchState>& in);

/*
 * Fabric switch test config
 */
cfg::SwitchConfig testConfigFabricSwitch();
/*
 * The returned configuration object, if applied to a SwitchState with ports
 * 1-20, will yield the same SwitchState as that returned by testStateA().
 */
cfg::SwitchConfig testConfigA(
    cfg::SwitchType switchType = cfg::SwitchType::NPU);
/*
 * Same as testConfgA but with AclLookupClass associated with every port.
 * (MH-NIC case queue-per-host configuration).
 */
cfg::SwitchConfig testConfigAWithLookupClasses();

/*
 * Create a SwitchState for testing.
 *
 * Profile B:
 *   - 10 ports, 1 VLAN, 1 interface
 *   - Ports 1-10 are in VLAN 1
 *   - Interface 1:
 *     - VLAN 1
 *     - MAC 00:02:00:00:00:01
 *     - IPs:
 *       10.0.0.1/24
 *       192.168.0.1/24
 *       2401:db00:2110:3001::0001/64
 */
std::shared_ptr<SwitchState> testStateB();

/*
 * Same as testStateA but with all ports
 * enabled and up
 */
std::shared_ptr<SwitchState> testStateBWithPortsUp();
/*
 * Convenience macro that wraps EXPECT_CALL() on the underlying MockHwSwitch
 */
#define EXPECT_HW_CALL(sw, method) EXPECT_CALL(*getMockHw(sw), method)

#define EXPECT_MANY_HW_CALLS(sw, method) \
  EXPECT_CALL(*getMockHw(sw), method).Times(testing::AtLeast(1))

/*
 * Convenience macro that wraps EXPECT_CALL() on the underlying MockPlatform
 */
#define EXPECT_PLATFORM_CALL(sw, method) \
  EXPECT_CALL(*getMockPlatform(sw), method)

/*
 * Helper functions for comparing buffer-like objects
 */
#define EXPECT_BUF_EQ(a, b) EXPECT_EQ(fbossHexDump(a), fbossHexDump(b))
#define ASSERT_BUF_EQ(a, b) ASSERT_EQ(fbossHexDump(a), fbossHexDump(b))

std::string fbossHexDump(const folly::IOBuf* buf);
std::string fbossHexDump(const folly::IOBuf& buf);
std::string fbossHexDump(folly::ByteRange buf);
std::string fbossHexDump(folly::StringPiece buf);
std::string fbossHexDump(const std::string& buf);

RouteNextHopSet makeNextHops(
    std::vector<std::string> ipStrs,
    std::optional<LabelForwardingAction> mplsAction = std::nullopt);

template <typename AddrT>
inline RouteNextHopSet makeNextHops(const std::vector<AddrT>& ips) {
  std::vector<std::string> ipStrs;
  std::for_each(ips.begin(), ips.end(), [&ipStrs](const auto& ip) {
    ipStrs.push_back(ip.str());
  });
  return makeNextHops(ipStrs);
}

RouteNextHopSet makeResolvedNextHops(
    std::vector<std::pair<InterfaceID, std::string>> intfAndIP,
    uint32_t weight = ECMP_WEIGHT);

RoutePrefixV4 makePrefixV4(std::string str);

RoutePrefixV6 makePrefixV6(std::string str);

/*
 * Convenience macro for expecting a packet to be transmitted by the switch
 *
 * The name should be a brief description of the match being performed.
 *
 * The matchFn should be a function that accepts a single "const TxPacket*"
 * argument.  If the packet matches it should return void, and if it doesn't
 * match it should throw an exception describing why the packet did not match.
 *
 * Note that you should not use gtest EXPECT_* macros when checking the
 * packet.  Just because one packet is not a match does not mean that another
 * packet will not arrive and match the filter.  For instance, consider the
 * following code:
 *
 *   EXPECT_SWITCHED_PKT(sw, "ARP reply", arpReplyChecker).Times(1);
 *   EXPECT_SWITCHED_PKT(sw, "ICMP packet", icmpChecker).Times(1);
 *
 * The code expects to see one ARP reply and one ICMP packet, but it may have
 * to call both filters on a packet to see which one it is.
 */
#define EXPECT_SWITCHED_PKT(sw, name, matchFn) \
  EXPECT_HW_CALL(                              \
      sw,                                      \
      sendPacketSwitchedAsync_(TxPacketMatcher::createMatcher(name, matchFn)))

#define EXPECT_MANY_SWITCHED_PKTS(sw, name, matchFn) \
  EXPECT_MANY_HW_CALLS(                              \
      sw,                                            \
      sendPacketSwitchedAsync_(TxPacketMatcher::createMatcher(name, matchFn)))

#define EXPECT_OUT_OF_PORT_PKT(sw, name, matchFn, portID, cos) \
  EXPECT_HW_CALL(                                              \
      sw,                                                      \
      sendPacketOutOfPortAsync_(                               \
          TxPacketMatcher::createMatcher(name, matchFn), portID, cos))

/**
 * Convenience macro for expecting RxPacket to be transmitted by TunManager.
 * usage:
 *  EXPECT_TUN_PKT(tunMgr, "Unicast Packet", packetChecker).Times(1)
 */
#define EXPECT_TUN_PKT(tun, name, dstIfID, matchFn) \
  EXPECT_CALL(                                      \
      *tun,                                         \
      sendPacketToHost_(                            \
          RxPacketMatcher::createMatcher(name, dstIfID, matchFn)))

/**
 * Templatized version of Matching function for Tx/Rx packet.
 */
template <typename T>
using MatchFn = std::function<void(const T* pkt)>;
using RxMatchFn = MatchFn<RxPacket>;
using TxMatchFn = MatchFn<TxPacket>;

/*
 * A gmock MatcherInterface for matching TxPacket objects.
 */
using TxPacketPtr = TxPacket*;
class TxPacketMatcher : public ::testing::MatcherInterface<TxPacketPtr> {
 public:
  TxPacketMatcher(folly::StringPiece name, TxMatchFn fn);

  static ::testing::Matcher<TxPacket*> createMatcher(
      folly::StringPiece name,
      TxMatchFn&& fn);

  bool MatchAndExplain(TxPacketPtr pkt, ::testing::MatchResultListener* l)
      const override;

  void DescribeTo(std::ostream* os) const override;
  void DescribeNegationTo(std::ostream* os) const override;

 private:
  const std::string name_;
  const TxMatchFn fn_;
};

/*
 * A gmock MatcherInterface for matching RxPacket objects.
 */
using RxMatchFnArgs = std::tuple<InterfaceID, std::shared_ptr<RxPacket>>;
class RxPacketMatcher : public ::testing::MatcherInterface<RxMatchFnArgs> {
 public:
  RxPacketMatcher(folly::StringPiece name, InterfaceID dstIfID, RxMatchFn fn);

  static ::testing::Matcher<RxMatchFnArgs>
  createMatcher(folly::StringPiece name, InterfaceID dstIfID, RxMatchFn&& fn);

  bool MatchAndExplain(RxMatchFnArgs args, ::testing::MatchResultListener* l)
      const override;

  void DescribeTo(std::ostream* os) const override;
  void DescribeNegationTo(std::ostream* os) const override;

 private:
  const std::string name_;
  const InterfaceID dstIfID_;
  const RxMatchFn fn_;
};

using SwitchStatePredicate = folly::Function<bool(const StateDelta&)>;

class WaitForSwitchState : public StateObserver {
 public:
  WaitForSwitchState(
      SwSwitch* sw,
      SwitchStatePredicate predicate,
      const std::string& name);
  ~WaitForSwitchState();

  void stateUpdated(const StateDelta& delta) override;
  template <class Rep = int64_t, class Period = std::ratio<1>>
  bool wait(
      const std::chrono::duration<Rep, Period>& timeout =
          std::chrono::seconds(5)) {
    std::unique_lock<std::mutex> lock(mtx_);
    while (!done_) {
      if (cv_.wait_for(lock, timeout) == std::cv_status::timeout) {
        XLOG(ERR) << "timed out on " << name_;
        return false;
      }
    }
    done_ = false;
    // TSAN seems not smart enough to figure out that ~WaitForSwitchState() will
    // only be called after wait() is successfully done. Thus, suppress TSAN
    // data race warning starting from now until WaitForSwitchState is
    // destroyed.
    tsanGuard_ =
        std::make_unique<folly::annotate_ignore_thread_sanitizer_guard>(
            __FILE__, __LINE__);
    return true;
  }

 private:
  SwitchStatePredicate predicate_;
  std::mutex mtx_;
  std::condition_variable cv_;
  bool done_{false};
  std::string name_;
  SwSwitch* sw_;
  std::unique_ptr<folly::annotate_ignore_thread_sanitizer_guard> tsanGuard_;
};

class WaitForMacEntryAddedOrDeleted : public WaitForSwitchState {
 public:
  WaitForMacEntryAddedOrDeleted(
      SwSwitch* sw,
      folly::MacAddress mac,
      VlanID vlan,
      bool added)
      : WaitForSwitchState(
            sw,
            [mac, vlan, added](const StateDelta& delta) {
              const auto& newVlan =
                  delta.getVlansDelta().getNew()->getNodeIf(vlan);

              auto newEntry = newVlan->getMacTable()->getMacIf(mac);
              if (added) {
                return (newEntry != nullptr);
              }
              return (newEntry == nullptr);
            },
            "WaitForMacEntryAddedOrDeleted") {}
  ~WaitForMacEntryAddedOrDeleted() {}
};

void programRoutes(
    const utility::RouteDistributionGenerator::RouteChunks& routeChunks,
    SwSwitch* sw);

std::vector<std::shared_ptr<Port>> getPortsInLoopbackMode(
    const std::shared_ptr<SwitchState>& state);

void preparedMockPortConfig(
    cfg::Port& portCfg,
    int32_t id,
    std::optional<std::string> name = std::nullopt,
    cfg::PortState state = cfg::PortState::ENABLED);

template <typename Node, bool areFields = false>
void validateNodeSerialization(const Node& node) {
  if constexpr (!areFields) {
    if constexpr (!kIsThriftCowNode<Node>) {
      // Thrifty Nodes
      auto nodeBack = Node::fromThrift(node.toThrift());
      EXPECT_EQ(node, *nodeBack);
    } else {
      // Thrift-cow Nodes
      auto nodeBack = std::make_shared<Node>();
      nodeBack->fromThrift(node.toThrift());
      EXPECT_EQ(node, *nodeBack);
    }
  } else {
    auto nodeBack = Node::fromThrift(node.toThrift());
    EXPECT_EQ(node, nodeBack);
  }
}

template <typename Node>
void validateThriftStructNodeSerialization(const Node& node) {
  auto nodeBack = std::make_shared<Node>(node.toThrift());
  EXPECT_EQ(node, *nodeBack);
  nodeBack = std::make_shared<Node>();
  nodeBack->fromThrift(node.toThrift());
  EXPECT_EQ(node, *nodeBack);
}

template <typename NodeMap>
bool isSameNodeMap(const NodeMap& lhs, const NodeMap& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }

  for (const auto& [key, value] : lhs.getAllNodes()) {
    if (auto other = rhs.getNodeIf(key); !other || *value != *other) {
      return false;
    }
  }
  return true;
}

template <typename NodeMap>
void validateNodeMapSerialization(const NodeMap& nodeMap) {
  auto nodeMapBack = NodeMap::fromThrift(nodeMap.toThrift());
  EXPECT_TRUE(isSameNodeMap(nodeMap, *nodeMapBack));
}

template <typename NodeMap>
void validateThriftMapMapSerialization(const NodeMap& nodeMap) {
  auto nodeMapBack = std::make_shared<NodeMap>(nodeMap.toThrift());
  EXPECT_TRUE(nodeMap.toThrift() == nodeMapBack->toThrift());
}
} // namespace facebook::fboss
