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

#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/TrafficPolicyUtils.h"

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

    queue.id_ref() = queueId;
    queue.name_ref() = "queue_per_host";
    queue.streamType_ref() = cfg::StreamType::UNICAST;
    queue.scheduling_ref() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
    queue.weight_ref() = kQueuePerHostWeight;
    portQueues.push_back(queue);
  }

  config->portQueueConfigs_ref()["queue_config"] = portQueues;

  for (auto& port : *config->ports_ref()) {
    port.portQueueConfigName_ref() = "queue_config";
    port.lookupClasses_ref() = kLookupClasses();
  }
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

void addQueuePerHostAcls(cfg::SwitchConfig* config) {
  cfg::Ttl ttl;
  std::tie(*ttl.value_ref(), *ttl.mask_ref()) = std::make_tuple(0x80, 0x80);
  auto ttlCounterName = getQueuePerHostTtlCounterName();

  utility::addTrafficCounter(config, ttlCounterName);

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
  ttlAcl->ttl_ref() = ttl;
  utility::addAclStat(config, getQueuePerHostTtlAclName(), ttlCounterName);
}

} // namespace facebook::fboss::utility
