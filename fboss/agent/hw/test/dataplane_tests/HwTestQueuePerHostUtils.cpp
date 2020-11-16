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
  static const std::vector<int> queueIds = {kQueuePerHostQueue0,
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

    queue.id = queueId;
    queue.name_ref() = "queue_per_host";
    queue.streamType = cfg::StreamType::UNICAST;
    queue.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
    queue.weight_ref() = kQueuePerHostWeight;
    portQueues.push_back(queue);
  }

  config->portQueueConfigs_ref()["queue_config"] = portQueues;

  for (auto& port : *config->ports_ref()) {
    port.portQueueConfigName_ref() = "queue_config";
    port.lookupClasses_ref() = kLookupClasses();
  }
}

std::string getQueuePerHostAclNameForQueue(int queueId) {
  return folly::to<std::string>("queue-per-host-queue-", queueId);
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

void addQueuePerHostAcls(cfg::SwitchConfig* config) {
  for (auto queueId : kQueuePerhostQueueIds()) {
    auto classID = kQueuePerHostQueueToClass().at(queueId);

    auto aclName = getQueuePerHostAclNameForQueue(queueId);
    utility::addClassIDAcl(config, aclName, classID);
    utility::addQueueMatcher(config, aclName, queueId);

    auto l2AclName = getQueuePerHostL2AclNameForQueue(queueId);
    utility::addL2ClassIDAcl(config, l2AclName, classID);
    utility::addQueueMatcher(config, l2AclName, queueId);

    auto neighborAclName = getQueuePerHostNeighborAclNameForQueue(queueId);
    utility::addNeighborClassIDAcl(config, neighborAclName, classID);
    utility::addQueueMatcher(config, neighborAclName, queueId);

    auto routeAclName = getQueuePerHostRouteAclNameForQueue(queueId);
    utility::addRouteClassIDAcl(config, routeAclName, classID);
    utility::addQueueMatcher(config, routeAclName, queueId);
  }
}

} // namespace facebook::fboss::utility
