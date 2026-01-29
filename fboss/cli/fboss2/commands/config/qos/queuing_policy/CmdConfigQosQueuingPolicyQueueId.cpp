/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/qos/queuing_policy/CmdConfigQosQueuingPolicyQueueId.h"

#include <fmt/format.h>
#include <folly/Conv.h>
#include <folly/String.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/commands/config/qos/queuing_policy/CmdConfigQosQueuingPolicy.h"
#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

namespace {

std::string getValidScalingFactors() {
  std::vector<std::string> names;
  for (auto value :
       apache::thrift::TEnumTraits<cfg::MMUScalingFactor>::values) {
    names.push_back(apache::thrift::util::enumNameSafe(value));
  }
  return folly::join(", ", names);
}

std::string getValidSchedulingTypes() {
  std::vector<std::string> names;
  for (auto value : apache::thrift::TEnumTraits<cfg::QueueScheduling>::values) {
    names.push_back(apache::thrift::util::enumNameSafe(value));
  }
  return folly::join(", ", names) + " (or short names: WRR, SP, DRR)";
}

std::string getValidStreamTypes() {
  std::vector<std::string> names;
  for (auto value : apache::thrift::TEnumTraits<cfg::StreamType>::values) {
    names.push_back(apache::thrift::util::enumNameSafe(value));
  }
  return folly::join(", ", names);
}

std::string getValidCongestionBehaviors() {
  std::vector<std::string> names;
  for (auto value :
       apache::thrift::TEnumTraits<cfg::QueueCongestionBehavior>::values) {
    names.push_back(apache::thrift::util::enumNameSafe(value));
  }
  return folly::join(", ", names);
}

// Convert to uppercase and replace dashes with underscores.
// This allows users to type "strict-priority" instead of "STRICT_PRIORITY".
std::string toUpper(const std::string& value) {
  std::string result = value;
  std::transform(
      result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return c == '-' ? '_' : std::toupper(c);
      });
  return result;
}

// Parse active-queue-management sub-attributes and update the AQM config.
// The aqmArgs are everything after "active-queue-management" keyword.
// Expected formats:
//   congestion-behavior <value>
//   detection linear <attr> <value> [<attr> <value> ...]
// where linear attrs are: minimum-length, maximum-length, probability
void parseAqmAttributes(
    const std::vector<std::string>& aqmArgs,
    cfg::ActiveQueueManagement& aqm) {
  if (aqmArgs.empty()) {
    throw std::invalid_argument(
        "active-queue-management requires sub-attributes: "
        "congestion-behavior <value> or detection linear <attr> <value> ...");
  }

  const size_t numArgs = aqmArgs.size();
  size_t i = 0;
  while (i < numArgs) {
    const auto& subAttr = aqmArgs[i];

    if (subAttr == "congestion-behavior") {
      if (i + 1 >= numArgs) {
        throw std::invalid_argument(
            fmt::format(
                "congestion-behavior requires a value. Valid values are: {}",
                getValidCongestionBehaviors()));
      }
      cfg::QueueCongestionBehavior behavior{};
      if (!apache::thrift::TEnumTraits<cfg::QueueCongestionBehavior>::findValue(
              toUpper(aqmArgs[i + 1]), &behavior)) {
        throw std::invalid_argument(
            fmt::format(
                "Invalid congestion-behavior: '{}'. Valid values are: {}",
                aqmArgs[i + 1],
                getValidCongestionBehaviors()));
      }
      aqm.behavior() = behavior;
      i += 2;
    } else if (subAttr == "detection") {
      if (i + 1 >= numArgs) {
        throw std::invalid_argument(
            "detection requires a type. Currently supported: linear");
      }
      const auto& detectionType = aqmArgs[i + 1];
      if (toUpper(detectionType) != "LINEAR") {
        throw std::invalid_argument(
            fmt::format(
                "Invalid detection type: '{}'. Currently supported: linear",
                detectionType));
      }
      i += 2;

      // Parse linear detection attributes
      cfg::LinearQueueCongestionDetection linear;
      if (aqm.detection().has_value() &&
          aqm.detection()->linear().has_value()) {
        linear = *aqm.detection()->linear();
      }

      while (i < numArgs) {
        const auto& linearAttr = aqmArgs[i];
        // Check if this is a new top-level AQM attribute
        if (linearAttr == "congestion-behavior") {
          break; // Let the outer loop handle it
        }
        if (i + 1 >= numArgs) {
          throw std::invalid_argument(
              fmt::format(
                  "Linear detection attribute '{}' requires a value. "
                  "Valid attributes are: minimum-length, maximum-length, probability",
                  linearAttr));
        }
        const auto& linearValue = aqmArgs[i + 1];

        if (linearAttr == "minimum-length") {
          int32_t val = folly::to<int32_t>(linearValue);
          if (val < 0) {
            throw std::invalid_argument(
                fmt::format(
                    "minimum-length must be non-negative, got: {}",
                    linearValue));
          }
          linear.minimumLength() = val;
        } else if (linearAttr == "maximum-length") {
          int32_t val = folly::to<int32_t>(linearValue);
          if (val < 0) {
            throw std::invalid_argument(
                fmt::format(
                    "maximum-length must be non-negative, got: {}",
                    linearValue));
          }
          linear.maximumLength() = val;
        } else if (linearAttr == "probability") {
          int32_t val = folly::to<int32_t>(linearValue);
          if (val < 0) {
            throw std::invalid_argument(
                fmt::format(
                    "probability must be non-negative, got: {}", linearValue));
          }
          linear.probability() = val;
        } else {
          throw std::invalid_argument(
              fmt::format(
                  "Unknown linear detection attribute: '{}'. "
                  "Valid attributes are: minimum-length, maximum-length, probability",
                  linearAttr));
        }
        i += 2;
      }

      cfg::QueueCongestionDetection detection;
      detection.linear() = linear;
      aqm.detection() = detection;
    } else {
      throw std::invalid_argument(
          fmt::format(
              "Unknown active-queue-management sub-attribute: '{}'. "
              "Valid sub-attributes are: congestion-behavior, detection",
              subAttr));
    }
  }
}

// Map short names to full enum names for scheduling
std::optional<cfg::QueueScheduling> parseScheduling(const std::string& value) {
  // Convert to uppercase for case-insensitive matching
  std::string upperValue = toUpper(value);

  // Try short names first
  static const std::map<std::string, cfg::QueueScheduling> shortNames = {
      {"WRR", cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN},
      {"SP", cfg::QueueScheduling::STRICT_PRIORITY},
      {"DRR", cfg::QueueScheduling::DEFICIT_ROUND_ROBIN},
  };

  auto it = shortNames.find(upperValue);
  if (it != shortNames.end()) {
    return it->second;
  }

  // Try full enum name
  cfg::QueueScheduling scheduling{};
  if (apache::thrift::TEnumTraits<cfg::QueueScheduling>::findValue(
          upperValue, &scheduling)) {
    return scheduling;
  }

  return std::nullopt;
}

} // namespace

QueueConfig::QueueConfig(std::vector<std::string> v) {
  // Minimum: <queue-id> <attr> <value>
  if (v.empty()) {
    throw std::invalid_argument(
        "Expected: <queue-id> <attr> <value> [<attr> <value> ...] where <attr> is one of: "
        "reserved-bytes, shared-bytes, weight, scaling-factor, scheduling, stream-type, "
        "buffer-pool-name, active-queue-management");
  }

  // Parse the queue ID (first argument)
  // TODO: What's the upper bound, the maximum queue ID seems ASIC dependent?
  const int16_t maxQueueId = 128; // Arbitrary but high enough limit
  queueId_ = folly::to<int16_t>(v[0]);
  if (queueId_ < 0 || queueId_ > maxQueueId) {
    throw std::invalid_argument(
        fmt::format(
            "Queue ID must be between 0 and {}, got: {}",
            maxQueueId,
            queueId_));
  }
  data_.push_back(v[0]);

  // Parse the remaining arguments
  // Most attributes are simple key-value pairs, but "active-queue-management"
  // has nested sub-attributes that consume all remaining arguments.
  for (size_t i = 1; i < v.size();) {
    const auto& attr = v[i];
    data_.push_back(attr);

    if (attr == "active-queue-management" || attr == "aqm") {
      // Everything after "active-queue-management" is part of the AQM config
      std::vector<std::string> aqmArgs;
      for (size_t j = i + 1; j < v.size(); ++j) {
        aqmArgs.push_back(v[j]);
        data_.push_back(v[j]);
      }
      aqmAttributes_ = std::move(aqmArgs);
      break; // AQM consumes all remaining arguments
    }

    // Regular key-value pair
    if (i + 1 >= v.size()) {
      throw std::invalid_argument(
          fmt::format("Attribute '{}' requires a value.", attr));
    }
    const auto& value = v[i + 1];
    attributes_.emplace_back(attr, value);
    data_.push_back(value);
    i += 2;
  }
}

CmdConfigQosQueuingPolicyQueueIdTraits::RetType
CmdConfigQosQueuingPolicyQueueId::queryClient(
    const HostInfo& /* hostInfo */,
    const QueuingPolicyName& policyName,
    const ObjectArgType& config) {
  auto& session = ConfigSession::getInstance();
  auto& agentConfig = session.getAgentConfig();
  auto& switchConfig = *agentConfig.sw();

  // Get or create the portQueueConfigs map
  auto& portQueueConfigs = *switchConfig.portQueueConfigs();
  int16_t queueIdVal = config.getQueueId();

  // Get or create the policy entry (list of PortQueue)
  auto& configList = portQueueConfigs[policyName.getName()];

  // Find the PortQueue with the matching queue ID, or create a new one
  cfg::PortQueue* targetConfig = nullptr;
  for (auto& queueConfig : configList) {
    if (*queueConfig.id() == queueIdVal) {
      targetConfig = &queueConfig;
      break;
    }
  }

  if (targetConfig == nullptr) {
    // Create a new PortQueue with the given queue ID
    cfg::PortQueue newConfig;
    newConfig.id() = queueIdVal;
    newConfig.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
    configList.push_back(newConfig);
    targetConfig = &configList.back();
  }

  // Process each attribute-value pair
  for (const auto& [attr, value] : config.getAttributes()) {
    if (attr == "reserved-bytes") {
      int32_t bytes = folly::to<int32_t>(value);
      if (bytes < 0) {
        throw std::invalid_argument(
            "reserved-bytes must be non-negative, got: " + value);
      }
      targetConfig->reservedBytes() = bytes;
    } else if (attr == "shared-bytes") {
      int32_t bytes = folly::to<int32_t>(value);
      if (bytes < 0) {
        throw std::invalid_argument(
            "shared-bytes must be non-negative, got: " + value);
      }
      targetConfig->sharedBytes() = bytes;
    } else if (attr == "weight") {
      int32_t weight = folly::to<int32_t>(value);
      if (weight < 0) {
        throw std::invalid_argument(
            "weight must be non-negative, got: " + value);
      }
      targetConfig->weight() = weight;
    } else if (attr == "scaling-factor") {
      cfg::MMUScalingFactor factor{};
      if (!apache::thrift::TEnumTraits<cfg::MMUScalingFactor>::findValue(
              toUpper(value), &factor)) {
        throw std::invalid_argument(
            "Invalid scaling-factor: '" + value +
            "'. Valid values are: " + getValidScalingFactors());
      }
      targetConfig->scalingFactor() = factor;
    } else if (attr == "scheduling") {
      auto scheduling = parseScheduling(value);
      if (!scheduling) {
        throw std::invalid_argument(
            "Invalid scheduling: '" + value +
            "'. Valid values are: " + getValidSchedulingTypes());
      }
      targetConfig->scheduling() = *scheduling;
    } else if (attr == "stream-type") {
      cfg::StreamType streamType{};
      if (!apache::thrift::TEnumTraits<cfg::StreamType>::findValue(
              toUpper(value), &streamType)) {
        throw std::invalid_argument(
            "Invalid stream-type: '" + value +
            "'. Valid values are: " + getValidStreamTypes());
      }
      targetConfig->streamType() = streamType;
    } else if (attr == "buffer-pool-name") {
      targetConfig->bufferPoolName() = value;
    } else {
      throw std::invalid_argument(
          "Unknown attribute: '" + attr +
          "'. Valid attributes are: reserved-bytes, shared-bytes, weight, "
          "scaling-factor, scheduling, stream-type, buffer-pool-name, "
          "active-queue-management");
    }
  }

  // Process active-queue-management attributes if present
  const auto& aqmArgs = config.getAqmAttributes();
  if (!aqmArgs.empty()) {
    // Get or create the AQM entry in the aqms list
    // For now, we only support a single AQM entry per queue
    if (!targetConfig->aqms().has_value() || targetConfig->aqms()->empty()) {
      targetConfig->aqms() = std::vector<cfg::ActiveQueueManagement>{};
      targetConfig->aqms()->emplace_back();
    }
    auto& aqm = targetConfig->aqms()->front();
    parseAqmAttributes(aqmArgs, aqm);
  }

  // Save the updated config
  session.saveConfig(cli::ConfigActionLevel::AGENT_RESTART);

  return fmt::format(
      "Successfully configured queuing-policy '{}' queue-id {}",
      policyName.getName(),
      queueIdVal);
}

void CmdConfigQosQueuingPolicyQueueId::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

} // namespace facebook::fboss
