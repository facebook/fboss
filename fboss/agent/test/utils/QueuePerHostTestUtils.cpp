/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/test/utils/QueuePerHostTestUtils.h"

#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/utils/AclTestUtils.h"

#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/TrafficPolicyTestUtils.h"

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

void addQueuePerHostAcls(cfg::SwitchConfig* config, bool isSai) {
  cfg::Ttl ttl;
  std::tie(*ttl.value(), *ttl.mask()) = std::make_tuple(0x80, 0x80);
  auto ttlCounterName = getQueuePerHostTtlCounterName();

  std::vector<cfg::CounterType> counterTypes{
      cfg::CounterType::PACKETS, cfg::CounterType::BYTES};
  utility::addTrafficCounter(config, ttlCounterName, counterTypes);

  // DENY rules
  utility::addL2ClassIDDropAcl(
      config, getL2DropAclName(), cfg::AclLookupClass::CLASS_DROP);
  utility::addNeighborClassIDDropAcl(
      config, getNeighborDropAclName(), cfg::AclLookupClass::CLASS_DROP);
  utility::addRouteClassIDDropAcl(
      config, getRouteDropAclName(), cfg::AclLookupClass::CLASS_DROP);

  // TTL + {L2, neighbor, route}
  for (auto queueId : kQueuePerhostQueueIds()) {
    auto classID = kQueuePerHostQueueToClass().at(queueId);

    auto l2AndTtlAclName = folly::to<std::string>(
        "ttl-", getQueuePerHostL2AclNameForQueue(queueId));
    utility::addL2ClassIDAndTtlAcl(config, l2AndTtlAclName, classID, ttl);
    utility::addQueueMatcher(
        config, l2AndTtlAclName, queueId, isSai, ttlCounterName);

    auto neighborAndTtlAclName = folly::to<std::string>(
        "ttl-", getQueuePerHostNeighborAclNameForQueue(queueId));
    utility::addNeighborClassIDAndTtlAcl(
        config, neighborAndTtlAclName, classID, ttl);
    utility::addQueueMatcher(
        config, neighborAndTtlAclName, queueId, isSai, ttlCounterName);

    auto routeAndTtlAclName = folly::to<std::string>(
        "ttl-", getQueuePerHostRouteAclNameForQueue(queueId));
    utility::addRouteClassIDAndTtlAcl(config, routeAndTtlAclName, classID, ttl);
    utility::addQueueMatcher(
        config, routeAndTtlAclName, queueId, isSai, ttlCounterName);
  }

  // {L2, neighbor, route}-only
  for (auto queueId : kQueuePerhostQueueIds()) {
    auto classID = kQueuePerHostQueueToClass().at(queueId);

    auto l2AclName = getQueuePerHostL2AclNameForQueue(queueId);
    utility::addL2ClassIDAndTtlAcl(config, l2AclName, classID);
    utility::addQueueMatcher(config, l2AclName, queueId, isSai);

    auto neighborAclName = getQueuePerHostNeighborAclNameForQueue(queueId);
    utility::addNeighborClassIDAndTtlAcl(config, neighborAclName, classID);
    utility::addQueueMatcher(config, neighborAclName, queueId, isSai);

    auto routeAclName = getQueuePerHostRouteAclNameForQueue(queueId);
    utility::addRouteClassIDAndTtlAcl(config, routeAclName, classID);
    utility::addQueueMatcher(config, routeAclName, queueId, isSai);
  }

  // TTL only
  auto* ttlAcl = utility::addAcl(config, getQueuePerHostTtlAclName());
  ttlAcl->ttl() = ttl;
  std::vector<cfg::CounterType> setCounterTypes{
      cfg::CounterType::PACKETS, cfg::CounterType::BYTES};

  utility::addAclStat(
      config, getQueuePerHostTtlAclName(), ttlCounterName, setCounterTypes);
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
void addTtlAclTable(
    cfg::SwitchConfig* config,
    int16_t priority,
    bool addExtraQualifier) {
  std::vector<cfg::AclTableQualifier> qualifiers = {
      cfg::AclTableQualifier::TTL, cfg::AclTableQualifier::DSCP};
  if (addExtraQualifier) {
    /*
     * This field is used to modify the properties of the ACL table.
     * This will force a recreate of the acl table during delta processing.
     */
    qualifiers.push_back(cfg::AclTableQualifier::OUT_PORT);
  }
  utility::addAclTable(
      config,
      getTtlAclTableName(),
      priority, // priority
      {cfg::AclTableActionType::PACKET_ACTION,
       cfg::AclTableActionType::COUNTER},
      qualifiers);

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
    const std::string& aclTableName,
    bool isSai) {
  for (auto queueId : kQueuePerhostQueueIds()) {
    auto classID = kQueuePerHostQueueToClass().at(queueId);

    auto l2AclName = getQueuePerHostL2AclNameForQueue(queueId);
    auto aclL2 = utility::addAcl(
        config, l2AclName, cfg::AclActionType::PERMIT, aclTableName);
    aclL2->lookupClassL2() = classID;
    utility::addQueueMatcher(config, l2AclName, queueId, isSai);

    auto neighborAclName = getQueuePerHostNeighborAclNameForQueue(queueId);
    auto aclNeighbor = utility::addAcl(
        config, neighborAclName, cfg::AclActionType::PERMIT, aclTableName);
    aclNeighbor->lookupClassNeighbor() = classID;
    utility::addQueueMatcher(config, neighborAclName, queueId, isSai);

    auto routeAclName = getQueuePerHostRouteAclNameForQueue(queueId);
    auto aclRoute = utility::addAcl(
        config, routeAclName, cfg::AclActionType::PERMIT, aclTableName);
    aclRoute->lookupClassRoute() = classID;
    utility::addQueueMatcher(config, routeAclName, queueId, isSai);
  }
}

// Utility to add {L2, neighbor, route}-Acl Table to Multi Acl table group
void addQueuePerHostAclTables(
    cfg::SwitchConfig* config,
    int16_t priority,
    bool addAllQualifiers,
    bool isSai) {
  std::vector<cfg::AclTableQualifier> qualifiers = {
      cfg::AclTableQualifier::LOOKUP_CLASS_L2,
      cfg::AclTableQualifier::LOOKUP_CLASS_NEIGHBOR,
      cfg::AclTableQualifier::LOOKUP_CLASS_ROUTE};
  std::vector<cfg::AclTableActionType> actions = {
      cfg::AclTableActionType::PACKET_ACTION, cfg::AclTableActionType::SET_TC};

  if (addAllQualifiers) {
    // Add the extra qualifiers needed for Cisco Key profile
    qualifiers.push_back(cfg::AclTableQualifier::TTL);
    qualifiers.push_back(cfg::AclTableQualifier::DSCP);
    actions.push_back(cfg::AclTableActionType::COUNTER);
  }

  utility::addAclTable(
      config,
      getQueuePerHostAclTableName(),
      priority, // priority
      actions,
      qualifiers);

  addQueuePerHostAclEntry(config, getQueuePerHostAclTableName(), isSai);
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
} // namespace facebook::fboss::utility
