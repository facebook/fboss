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

void addQueuePerHostQueueConfig(cfg::SwitchConfig* config, PortID portID) {
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
  auto portCfg = std::find_if(
      config->ports_ref()->begin(),
      config->ports_ref()->end(),
      [&portID](auto& port) {
        return PortID(*port.logicalID_ref()) == portID;
      });
  portCfg->portQueueConfigName_ref() = "queue_config";
  portCfg->lookupClasses_ref() = kLookupClasses();
}

std::string getQueuePerHostAclNameForQueue(int queueId) {
  return folly::to<std::string>("queue-per-host-queue-", queueId);
}

void addQueuePerHostAcls(cfg::SwitchConfig* config) {
  for (auto queueId : kQueuePerhostQueueIds()) {
    auto aclName = getQueuePerHostAclNameForQueue(queueId);
    auto classID = kQueuePerHostQueueToClass().at(queueId);

    utility::addClassIDAclToCfg(config, aclName, classID);
    utility::addQueueMatcher(config, aclName, queueId);
  }
}

} // namespace facebook::fboss::utility
