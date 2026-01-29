/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/qos/policy/CmdConfigQosPolicyMap.h"

#include <fmt/format.h>
#include <folly/Conv.h>
#include <folly/lang/Assume.h>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/commands/config/qos/policy/CmdConfigQosPolicy.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

namespace {

constexpr int16_t kMinValue = 0;
constexpr int16_t kMaxValue = 7;

std::string getMapTypeString(QosMapType mapType) {
  switch (mapType) {
    case QosMapType::TC_TO_QUEUE:
      return "trafficClassToQueueId";
    case QosMapType::PFC_PRI_TO_QUEUE:
      return "pfcPriorityToQueueId";
    case QosMapType::TC_TO_PG:
      return "trafficClassToPgId";
    case QosMapType::PFC_PRI_TO_PG:
      return "pfcPriorityToPgId";
  }
  folly::assume_unreachable();
}

} // namespace

QosMapConfig::QosMapConfig(std::vector<std::string> v) {
  // Expected format: <map-type> <key> <value>
  if (v.size() < 3) {
    throw std::invalid_argument(
        "Expected: <map-type> <key> <value> where map-type is one of: "
        "tc-to-queue, pfc-pri-to-queue, tc-to-pg, pfc-pri-to-pg");
  }

  // Parse the map type
  const auto& mapTypeStr = v[0];
  if (mapTypeStr == "tc-to-queue") {
    mapType_ = QosMapType::TC_TO_QUEUE;
  } else if (mapTypeStr == "pfc-pri-to-queue") {
    mapType_ = QosMapType::PFC_PRI_TO_QUEUE;
  } else if (mapTypeStr == "tc-to-pg") {
    mapType_ = QosMapType::TC_TO_PG;
  } else if (mapTypeStr == "pfc-pri-to-pg") {
    mapType_ = QosMapType::PFC_PRI_TO_PG;
  } else {
    throw std::invalid_argument(
        fmt::format(
            "Invalid map type: '{}'. Valid values are: "
            "tc-to-queue, pfc-pri-to-queue, tc-to-pg, pfc-pri-to-pg",
            mapTypeStr));
  }
  data_.push_back(mapTypeStr);

  // Parse the key
  key_ = folly::to<int16_t>(v[1]);
  if (key_ < kMinValue || key_ > kMaxValue) {
    throw std::invalid_argument(
        fmt::format(
            "Key must be between {} and {}, got: {}",
            kMinValue,
            kMaxValue,
            key_));
  }
  data_.push_back(v[1]);

  // Parse the value
  value_ = folly::to<int16_t>(v[2]);
  if (value_ < kMinValue || value_ > kMaxValue) {
    throw std::invalid_argument(
        fmt::format(
            "Value must be between {} and {}, got: {}",
            kMinValue,
            kMaxValue,
            value_));
  }
  data_.push_back(v[2]);
}

CmdConfigQosPolicyMapTraits::RetType CmdConfigQosPolicyMap::queryClient(
    const HostInfo& /* hostInfo */,
    const QosPolicyName& policyName,
    const ObjectArgType& config) {
  auto& session = ConfigSession::getInstance();
  auto& agentConfig = session.getAgentConfig();
  auto& switchConfig = *agentConfig.sw();

  const std::string& name = policyName.getName();
  auto& qosPolicies = *switchConfig.qosPolicies();

  // Find or create the QosPolicy with the given name
  cfg::QosPolicy* targetPolicy = nullptr;
  for (auto& policy : qosPolicies) {
    if (*policy.name() == name) {
      targetPolicy = &policy;
      break;
    }
  }

  if (targetPolicy == nullptr) {
    // Create a new QosPolicy
    cfg::QosPolicy newPolicy;
    newPolicy.name() = name;
    qosPolicies.push_back(std::move(newPolicy));
    targetPolicy = &qosPolicies.back();
  }

  // Ensure qosMap is initialized
  if (!targetPolicy->qosMap().has_value()) {
    targetPolicy->qosMap() = cfg::QosMap();
  }
  auto& qosMap = *targetPolicy->qosMap();

  // Set the appropriate map entry based on map type
  QosMapType mapType = config.getMapType();
  int16_t key = config.getKey();
  int16_t value = config.getValue();

  switch (mapType) {
    case QosMapType::TC_TO_QUEUE:
      (*qosMap.trafficClassToQueueId())[key] = value;
      break;
    case QosMapType::PFC_PRI_TO_QUEUE:
      if (!qosMap.pfcPriorityToQueueId().has_value()) {
        qosMap.pfcPriorityToQueueId() = std::map<int16_t, int16_t>();
      }
      (*qosMap.pfcPriorityToQueueId())[key] = value;
      break;
    case QosMapType::TC_TO_PG:
      if (!qosMap.trafficClassToPgId().has_value()) {
        qosMap.trafficClassToPgId() = std::map<int16_t, int16_t>();
      }
      (*qosMap.trafficClassToPgId())[key] = value;
      break;
    case QosMapType::PFC_PRI_TO_PG:
      if (!qosMap.pfcPriorityToPgId().has_value()) {
        qosMap.pfcPriorityToPgId() = std::map<int16_t, int16_t>();
      }
      (*qosMap.pfcPriorityToPgId())[key] = value;
      break;
  }

  session.saveConfig();

  return fmt::format(
      "Successfully set QoS policy '{}' {} [{}] = {}",
      name,
      getMapTypeString(mapType),
      key,
      value);
}

void CmdConfigQosPolicyMap::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

} // namespace facebook::fboss
