/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/delete/qos/policy/CmdDeleteQosPolicyMap.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/Conv.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/commands/config/qos/QosPolicyUtils.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

namespace {
constexpr auto kSupportedMapTypes = "dscp, tc-to-queue";
} // namespace

DeleteQosMapEntry::DeleteQosMapEntry(std::vector<std::string> v) {
  if (v.size() != 2) {
    throw std::invalid_argument(
        fmt::format(
            "Expected '<map-type> <value>', got {} argument(s). "
            "Valid map types: {}",
            v.size(),
            kSupportedMapTypes));
  }

  const std::string& mapType = v[0];
  if (mapType == "dscp") {
    mapType_ = DeleteQosMapType::DSCP;
  } else if (mapType == "tc-to-queue") {
    mapType_ = DeleteQosMapType::TC_TO_QUEUE;
  } else {
    throw std::invalid_argument(
        fmt::format(
            "Unknown map type '{}'. Valid map types: {}",
            mapType,
            kSupportedMapTypes));
  }

  auto parsed = folly::tryTo<int16_t>(v[1]);
  if (!parsed.hasValue()) {
    throw std::invalid_argument(
        fmt::format("Invalid value '{}'. Must be an integer", v[1]));
  }
  key_ = parsed.value();

  if (mapType_ == DeleteQosMapType::DSCP) {
    if (key_ < utils::kMinDscp || key_ > utils::kMaxDscp) {
      throw std::invalid_argument(
          fmt::format(
              "dscp value must be between {} and {}, got: {}",
              utils::kMinDscp,
              utils::kMaxDscp,
              v[1]));
    }
  } else if (key_ < 0) {
    throw std::invalid_argument(
        fmt::format("traffic class must not be negative, got: {}", v[1]));
  }

  data_ = std::move(v);
}

namespace {

// Removes `dscp` from whichever DscpQosMap lists it. An entry that ends up
// with no ingress codepoints and no egress rewrite carries no information, so
// it is dropped rather than left behind as an empty shell.
bool eraseDscp(std::vector<cfg::DscpQosMap>& dscpMaps, int16_t dscp) {
  auto byteVal = static_cast<int8_t>(dscp);
  bool erased = false;

  for (auto& entry : dscpMaps) {
    auto& fromList = *entry.fromDscpToTrafficClass();
    auto it = std::find(fromList.begin(), fromList.end(), byteVal);
    if (it != fromList.end()) {
      fromList.erase(it);
      erased = true;
    }
  }

  if (erased) {
    dscpMaps.erase(
        std::remove_if(
            dscpMaps.begin(),
            dscpMaps.end(),
            [](const cfg::DscpQosMap& entry) {
              return entry.fromDscpToTrafficClass()->empty() &&
                  !entry.fromTrafficClassToDscp().has_value();
            }),
        dscpMaps.end());
  }

  return erased;
}

} // namespace

CmdDeleteQosPolicyMapTraits::RetType CmdDeleteQosPolicyMap::queryClient(
    const HostInfo& /* hostInfo */,
    const QosPolicyName& policyName,
    const ObjectArgType& entry) {
  auto& session = ConfigSession::getInstance();
  auto& agentConfig = session.getAgentConfig();
  auto& switchConfig = *agentConfig.sw();

  const std::string name = policyName.getName();
  auto& qosPolicies = *switchConfig.qosPolicies();

  auto policyIt = utils::findQosPolicyOrThrow(qosPolicies, name);

  if (!policyIt->qosMap().has_value()) {
    throw std::runtime_error(
        fmt::format("QoS policy '{}' has no qosMap configured", name));
  }
  auto& qosMap = *policyIt->qosMap();

  switch (entry.getMapType()) {
    case DeleteQosMapType::DSCP: {
      if (!eraseDscp(*qosMap.dscpMaps(), entry.getKey())) {
        throw std::runtime_error(
            fmt::format(
                "QoS policy '{}' has no dscp mapping for value {}",
                name,
                entry.getKey()));
      }
      session.saveConfig();
      return fmt::format(
          "Successfully deleted QoS policy '{}' dscp mapping for value {}",
          name,
          entry.getKey());
    }
    case DeleteQosMapType::TC_TO_QUEUE: {
      auto& tcToQueue = *qosMap.trafficClassToQueueId();
      if (tcToQueue.erase(entry.getKey()) == 0) {
        throw std::runtime_error(
            fmt::format(
                "QoS policy '{}' has no tc-to-queue mapping for traffic class {}",
                name,
                entry.getKey()));
      }
      session.saveConfig();
      return fmt::format(
          "Successfully deleted QoS policy '{}' tc-to-queue mapping for traffic class {}",
          name,
          entry.getKey());
    }
  }

  throw std::runtime_error("Unhandled map type");
}

void CmdDeleteQosPolicyMap::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

template void
CmdHandler<CmdDeleteQosPolicyMap, CmdDeleteQosPolicyMapTraits>::run();

} // namespace facebook::fboss
