/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/dataplane_tests/HwTestQueuePerHostUtils.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/TrafficPolicyUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestUtils.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/logging/xlog.h>

#include <string>

namespace facebook::fboss::utility {

const std::map<int, cfg::AclLookupClass>& kQueuePerHostQueueToClass() {
  static const std::map<int, cfg::AclLookupClass> queueToClass = {
      {kQueuePerHostQueue0, cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0},
      {kQueuePerHostQueue1, cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1},
      {kQueuePerHostQueue2, cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2},
      {kQueuePerHostQueue3, cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3},
      {kQueuePerHostQueue4, cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_4},
  };

  return queueToClass;
}

const std::vector<int>& kQueuePerhostQueueIds() {
  static const std::vector<int> queueIds = {
      kQueuePerHostQueue0,
      kQueuePerHostQueue1,
      kQueuePerHostQueue2,
      kQueuePerHostQueue3,
      kQueuePerHostQueue4};

  return queueIds;
}

const std::vector<cfg::AclLookupClass>& kLookupClasses() {
  static const std::vector<cfg::AclLookupClass> lookupClasses = {
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3,
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_4};

  return lookupClasses;
}

void addQueuePerHostQueueConfig(cfg::SwitchConfig* config) {
  std::vector<cfg::PortQueue> portQueues;

  // All Queue-per-host queues are identical by design
  for (auto queueId : kQueuePerhostQueueIds()) {
    cfg::PortQueue queue;

    queue.id() = queueId;
    queue.name() = "queue_per_host";
    queue.streamType() = cfg::StreamType::UNICAST;
    queue.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
    queue.weight() = kQueuePerHostWeight;
    portQueues.push_back(queue);
  }

  config->portQueueConfigs()["queue_config"] = portQueues;

  for (auto& port : *config->ports()) {
    port.portQueueConfigName() = "queue_config";
    port.lookupClasses() = kLookupClasses();
  }
}

std::string getAclTableGroupName() {
  return "acl-table-group-ingress";
}

std::string getQueuePerHostAclTableName() {
  return "acl-table-queue-per-host";
}

std::string getTtlAclTableName() {
  return "acl-table-ttl";
}

std::string getQueuePerHostL2AclNameForQueue(int queueId) {
  return folly::to<std::string>("queue-per-host-queue-l2-", queueId);
}

std::string getQueuePerHostNeighborAclNameForQueue(int queueId) {
  return folly::to<std::string>("queue-per-host-queue-neighbor-", queueId);
}

std::string getQueuePerHostRouteAclNameForQueue(int queueId) {
  return folly::to<std::string>("queue-per-host-queue-route-", queueId);
}

std::string getQueuePerHostTtlAclName() {
  return "ttl";
}

std::string getQueuePerHostTtlCounterName() {
  return "ttlCounter";
}

std::string getL2DropAclName() {
  return "l2-drop-acl";
}

std::string getNeighborDropAclName() {
  return "neighbor-drop-acl";
}

std::string getRouteDropAclName() {
  return "route-drop-acl";
}

void addQueuePerHostAcls(cfg::SwitchConfig* config) {
  cfg::Ttl ttl;
  std::tie(*ttl.value(), *ttl.mask()) = std::make_tuple(0x80, 0x80);
  auto ttlCounterName = getQueuePerHostTtlCounterName();

  std::vector<cfg::CounterType> counterTypes{
      cfg::CounterType::PACKETS, cfg::CounterType::BYTES};
  utility::addTrafficCounter(config, ttlCounterName, counterTypes);

  // TTL + {L2, neighbor, route}
  for (auto queueId : kQueuePerhostQueueIds()) {
    auto classID = kQueuePerHostQueueToClass().at(queueId);

    auto l2AndTtlAclName = folly::to<std::string>(
        "ttl-", getQueuePerHostL2AclNameForQueue(queueId));
    utility::addL2ClassIDAndTtlAcl(config, l2AndTtlAclName, classID, ttl);
    utility::addQueueMatcher(config, l2AndTtlAclName, queueId, ttlCounterName);

    auto neighborAndTtlAclName = folly::to<std::string>(
        "ttl-", getQueuePerHostNeighborAclNameForQueue(queueId));
    utility::addNeighborClassIDAndTtlAcl(
        config, neighborAndTtlAclName, classID, ttl);
    utility::addQueueMatcher(
        config, neighborAndTtlAclName, queueId, ttlCounterName);

    auto routeAndTtlAclName = folly::to<std::string>(
        "ttl-", getQueuePerHostRouteAclNameForQueue(queueId));
    utility::addRouteClassIDAndTtlAcl(config, routeAndTtlAclName, classID, ttl);
    utility::addQueueMatcher(
        config, routeAndTtlAclName, queueId, ttlCounterName);
  }

  // {L2, neighbor, route}-only
  for (auto queueId : kQueuePerhostQueueIds()) {
    auto classID = kQueuePerHostQueueToClass().at(queueId);

    auto l2AclName = getQueuePerHostL2AclNameForQueue(queueId);
    utility::addL2ClassIDAndTtlAcl(config, l2AclName, classID);
    utility::addQueueMatcher(config, l2AclName, queueId);

    auto neighborAclName = getQueuePerHostNeighborAclNameForQueue(queueId);
    utility::addNeighborClassIDAndTtlAcl(config, neighborAclName, classID);
    utility::addQueueMatcher(config, neighborAclName, queueId);

    auto routeAclName = getQueuePerHostRouteAclNameForQueue(queueId);
    utility::addRouteClassIDAndTtlAcl(config, routeAclName, classID);
    utility::addQueueMatcher(config, routeAclName, queueId);
  }

  // TTL only
  auto* ttlAcl = utility::addAcl(config, getQueuePerHostTtlAclName());
  ttlAcl->ttl() = ttl;
  std::vector<cfg::CounterType> setCounterTypes{
      cfg::CounterType::PACKETS, cfg::CounterType::BYTES};

  utility::addAclStat(
      config, getQueuePerHostTtlAclName(), ttlCounterName, setCounterTypes);

  utility::addL2ClassIDDropAcl(
      config, getL2DropAclName(), cfg::AclLookupClass::CLASS_DROP);
  utility::addNeighborClassIDDropAcl(
      config, getNeighborDropAclName(), cfg::AclLookupClass::CLASS_DROP);
  utility::addRouteClassIDDropAcl(
      config, getRouteDropAclName(), cfg::AclLookupClass::CLASS_DROP);
}

void deleteTtlCounters(cfg::SwitchConfig* config) {
  auto ttlCounterName = getQueuePerHostTtlCounterName();

  utility::delAclStat(config, getQueuePerHostTtlAclName(), ttlCounterName);
}

void addTtlAclEntry(
    cfg::SwitchConfig* config,
    const std::string& aclTableName) {
  cfg::Ttl ttl;
  std::tie(*ttl.value(), *ttl.mask()) = std::make_tuple(0x80, 0x80);
  auto ttlCounterName = getQueuePerHostTtlCounterName();
  std::vector<cfg::CounterType> counterTypes{
      cfg::CounterType::PACKETS, cfg::CounterType::BYTES};
  utility::addTrafficCounter(config, ttlCounterName, counterTypes);

  auto* ttlAcl = utility::addAcl(
      config,
      getQueuePerHostTtlAclName(),
      cfg::AclActionType::PERMIT,
      aclTableName);
  ttlAcl->ttl() = ttl;
  std::vector<cfg::CounterType> setCounterTypes{
      cfg::CounterType::PACKETS, cfg::CounterType::BYTES};

  utility::addAclStat(
      config, getQueuePerHostTtlAclName(), ttlCounterName, setCounterTypes);
}

// Utility to add TTL ACL table to a multi acl table group
void addTtlAclTable(cfg::SwitchConfig* config, int16_t priority) {
  utility::addAclTable(
      config,
      getTtlAclTableName(),
      priority, // priority
      {cfg::AclTableActionType::PACKET_ACTION,
       cfg::AclTableActionType::COUNTER},
      {cfg::AclTableQualifier::TTL});

  addTtlAclEntry(config, getTtlAclTableName());
}

void deleteQueuePerHostMatchers(cfg::SwitchConfig* config) {
  for (auto queueId : kQueuePerhostQueueIds()) {
    auto l2AclName = getQueuePerHostL2AclNameForQueue(queueId);
    utility::delMatcher(config, l2AclName);

    auto neighborAclName = getQueuePerHostNeighborAclNameForQueue(queueId);
    utility::delMatcher(config, neighborAclName);

    auto routeAclName = getQueuePerHostRouteAclNameForQueue(queueId);
    utility::delMatcher(config, routeAclName);
  }
}

void addQueuePerHostAclEntry(
    cfg::SwitchConfig* config,
    const std::string& aclTableName) {
  for (auto queueId : kQueuePerhostQueueIds()) {
    auto classID = kQueuePerHostQueueToClass().at(queueId);

    auto l2AclName = getQueuePerHostL2AclNameForQueue(queueId);
    auto aclL2 = utility::addAcl(
        config, l2AclName, cfg::AclActionType::PERMIT, aclTableName);
    aclL2->lookupClassL2() = classID;
    utility::addQueueMatcher(config, l2AclName, queueId);

    auto neighborAclName = getQueuePerHostNeighborAclNameForQueue(queueId);
    auto aclNeighbor = utility::addAcl(
        config, neighborAclName, cfg::AclActionType::PERMIT, aclTableName);
    aclNeighbor->lookupClassNeighbor() = classID;
    utility::addQueueMatcher(config, neighborAclName, queueId);

    auto routeAclName = getQueuePerHostRouteAclNameForQueue(queueId);
    auto aclRoute = utility::addAcl(
        config, routeAclName, cfg::AclActionType::PERMIT, aclTableName);
    aclRoute->lookupClassRoute() = classID;
    utility::addQueueMatcher(config, routeAclName, queueId);
  }
}

// Utility to add {L2, neighbor, route}-Acl Table to Multi Acl table group
void addQueuePerHostAclTables(cfg::SwitchConfig* config, int16_t priority) {
  utility::addAclTable(
      config,
      getQueuePerHostAclTableName(),
      priority, // priority
      {cfg::AclTableActionType::PACKET_ACTION, cfg::AclTableActionType::SET_TC},
      {cfg::AclTableQualifier::LOOKUP_CLASS_L2,
       cfg::AclTableQualifier::LOOKUP_CLASS_NEIGHBOR,
       cfg::AclTableQualifier::LOOKUP_CLASS_ROUTE});

  addQueuePerHostAclEntry(config, getQueuePerHostAclTableName());
}

void verifyQueuePerHostMapping(
    HwSwitch* hwSwitch,
    std::shared_ptr<SwitchState> swState,
    const std::vector<PortID>& portIds,
    std::optional<VlanID> vlanId,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIp,
    const folly::IPAddress& dstIp,
    bool useFrontPanel,
    bool blockNeighbor,
    std::function<std::map<PortID, HwPortStats>(const std::vector<PortID>&)>
        getHwPortStatsFn,
    std::optional<uint16_t> l4SrcPort,
    std::optional<uint16_t> l4DstPort,
    std::optional<uint8_t> dscp) {
  auto ttlAclName = utility::getQueuePerHostTtlAclName();
  auto ttlCounterName = utility::getQueuePerHostTtlCounterName();

  auto statBefore = utility::getAclInOutPackets(
      hwSwitch, swState, ttlAclName, ttlCounterName);

  std::map<int, int64_t> beforeQueueOutPkts;
  for (const auto& queueId : utility::kQueuePerhostQueueIds()) {
    auto hwPortStatsMap = getHwPortStatsFn({portIds[0]});
    beforeQueueOutPkts[queueId] =
        hwPortStatsMap[portIds[0]].get_queueOutPackets_().at(queueId);
  }

  auto txPacket = utility::createUdpPkt(
      hwSwitch,
      vlanId,
      srcMac,
      dstMac,
      srcIp,
      dstIp,
      l4SrcPort.has_value() ? l4SrcPort.value() : 8000,
      l4DstPort.has_value() ? l4DstPort.value() : 8001,
      64 /* ttl < 128 */,
      dscp);
  auto txPacket2 = createUdpPkt(
      hwSwitch,
      vlanId,
      srcMac,
      dstMac,
      srcIp,
      dstIp,
      l4SrcPort.has_value() ? l4SrcPort.value() : 8000,
      l4DstPort.has_value() ? l4DstPort.value() : 8001,
      128 /* ttl >= 128 */,
      dscp);

  if (useFrontPanel) {
    utility::ensureSendPacketOutOfPort(
        hwSwitch,
        std::move(txPacket),
        PortID(portIds[1]),
        portIds,
        getHwPortStatsFn);
    utility::ensureSendPacketOutOfPort(
        hwSwitch,
        std::move(txPacket2),
        PortID(portIds[1]),
        portIds,
        getHwPortStatsFn);
  } else {
    utility::ensureSendPacketSwitched(
        hwSwitch, std::move(txPacket), portIds, getHwPortStatsFn);
    utility::ensureSendPacketSwitched(
        hwSwitch, std::move(txPacket2), portIds, getHwPortStatsFn);
  }

  std::map<int, int64_t> afterQueueOutPkts;
  for (const auto& queueId : utility::kQueuePerhostQueueIds()) {
    auto hwPortStatsMap = getHwPortStatsFn({portIds[0]});
    afterQueueOutPkts[queueId] =
        hwPortStatsMap[portIds[0]].get_queueOutPackets_().at(queueId);
  }

  /*
   *  Consider ACL with action to egress pkts through queue 2.
   *
   *  CPU originated packets:
   *     - Hits ACL (queue2Cnt = 1), egress through queue 2 of port0.
   *     - port0 is in loopback mode, so the packet gets looped back.
   *     - When packet is routed, its dstMAC gets overwritten. Thus, the
   *       looped back packet is not routed, and thus does not hit the ACL.
   *     - On some platforms, looped back packets for unknown MACs are
   *       flooded and counted on queue *before* the split horizon check
   *       (drop when srcPort == dstPort). This flooding always happens on
   *       queue 0, so expect one or more packets on queue 0.
   *
   *  Front panel packets (injected with pipeline bypass):
   *     - Egress out of port1 queue0 (pipeline bypass).
   *     - port1 is in loopback mode, so the packet gets looped back.
   *     - Rest of the workflow is same as above when CPU originated packet
   *       gets injected for switching.
   */
  for (auto [qid, beforePkts] : beforeQueueOutPkts) {
    auto pktsOnQueue = afterQueueOutPkts[qid] - beforePkts;

    XLOG(DBG2) << "queueId: " << qid << " pktsOnQueue: " << pktsOnQueue;

    if (blockNeighbor) {
      // if the neighbor is blocked, all pkts are dropped
      EXPECT_EQ(pktsOnQueue, 0);
    } else {
      if (qid == kQueueId) {
        EXPECT_EQ(pktsOnQueue, 2);
      } else if (qid == 0) {
        EXPECT_GE(pktsOnQueue, 0);
      } else {
        EXPECT_EQ(pktsOnQueue, 0);
      }
    }
  }

  auto statAfter = utility::getAclInOutPackets(
      hwSwitch, swState, ttlAclName, ttlCounterName);

  if (blockNeighbor) {
    // if the neighbor is blocked, all pkts are dropped
    EXPECT_EQ(statAfter - statBefore, 0);
  } else {
    /*
     * counts ttl >= 128 packet only
     */
    EXPECT_EQ(statAfter - statBefore, 1);
  }
}
void updateRoutesClassID(
    const std::map<
        RoutePrefix<folly::IPAddressV4>,
        std::optional<cfg::AclLookupClass>>& routePrefix2ClassID,
    RouteUpdateWrapper* updater) {
  for (const auto& [routePrefix, classID] : routePrefix2ClassID) {
    updater->programClassID(
        RouterID(0),
        {{folly::IPAddress(routePrefix.network()), routePrefix.mask()}},
        classID,
        false /* sync*/);
  }
}

void verifyQueuePerHostMapping(
    const HwSwitch* hwSwitch,
    HwSwitchEnsemble* ensemble,
    std::optional<VlanID> vlanId,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIp,
    const folly::IPAddress& dstIp,
    bool useFrontPanel,
    bool blockNeighbor,
    std::optional<uint16_t> l4SrcPort,
    std::optional<uint16_t> l4DstPort,
    std::optional<uint8_t> dscp) {
  // lambda that returns HwPortStats for the given port
  auto getPortStats =
      [&](const std::vector<PortID>& portIds) -> std::map<PortID, HwPortStats> {
    return ensemble->getLatestPortStats(portIds);
  };

  verifyQueuePerHostMapping(
      const_cast<HwSwitch*>(hwSwitch),
      ensemble->getProgrammedState(),
      ensemble->masterLogicalPortIds(),
      vlanId,
      srcMac,
      dstMac,
      srcIp,
      dstIp,
      useFrontPanel,
      blockNeighbor,
      getPortStats,
      l4SrcPort,
      l4DstPort,
      dscp);
}

} // namespace facebook::fboss::utility
