/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/qos/PortQueueConfigUtils.h"

#include <fmt/format.h>
#include <folly/Conv.h>
#include <folly/String.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>

namespace facebook::fboss::utils {

namespace {

// Uppercase and turn dashes into underscores so users can type
// "strict-priority" for the thrift enum STRICT_PRIORITY.
std::string toUpper(const std::string& value) {
  std::string result = value;
  std::transform(
      result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return c == '-' ? '_' : std::toupper(c);
      });
  return result;
}

template <typename EnumT>
std::string getValidEnumValues() {
  std::vector<std::string> names;
  for (auto value : apache::thrift::TEnumTraits<EnumT>::values) {
    names.push_back(apache::thrift::util::enumNameSafe(value));
  }
  return folly::join(", ", names);
}

template <typename EnumT>
EnumT parseThriftEnum(const std::string& attr, const std::string& value) {
  EnumT result{};
  if (!apache::thrift::TEnumTraits<EnumT>::findValue(toUpper(value), &result)) {
    throw std::invalid_argument(
        fmt::format(
            "Invalid {}: '{}'. Valid values are: {}",
            attr,
            value,
            getValidEnumValues<EnumT>()));
  }
  return result;
}

std::string getValidSchedulingTypes() {
  return getValidEnumValues<cfg::QueueScheduling>() +
      " (or short names: WRR, SP, DRR)";
}

int32_t parseNonNegativeInt(const std::string& attr, const std::string& value) {
  auto parsed = folly::tryTo<int32_t>(value);
  if (!parsed.hasValue()) {
    throw std::invalid_argument(
        fmt::format("{} must be an integer, got: {}", attr, value));
  }
  if (parsed.value() < 0) {
    throw std::invalid_argument(
        fmt::format("{} must be non-negative, got: {}", attr, value));
  }
  return parsed.value();
}

using LinearSetter = void (*)(cfg::LinearQueueCongestionDetection&, int32_t);

const std::map<std::string, LinearSetter>& linearSetters() {
  static const std::map<std::string, LinearSetter> kSetters = {
      {"minimum-length",
       [](cfg::LinearQueueCongestionDetection& l, int32_t v) {
         l.minimumLength() = v;
       }},
      {"maximum-length",
       [](cfg::LinearQueueCongestionDetection& l, int32_t v) {
         l.maximumLength() = v;
       }},
      {"probability",
       [](cfg::LinearQueueCongestionDetection& l, int32_t v) {
         l.probability() = v;
       }},
  };
  return kSetters;
}

std::string getValidLinearAttrs() {
  std::vector<std::string> keys;
  for (const auto& [key, setter] : linearSetters()) {
    keys.push_back(key);
  }
  return folly::join(", ", keys);
}

// Human-readable list of the supported top-level attributes, shared by the
// "at least one attribute" and "unknown attribute" error messages so the two
// can't drift apart.
const std::string& validTopLevelAttrs() {
  static const std::string kAttrs =
      "reserved-bytes, shared-bytes, weight, scaling-factor, scheduling, "
      "stream-type, buffer-pool-name, active-queue-management";
  return kAttrs;
}

std::optional<cfg::QueueScheduling> parseScheduling(const std::string& value) {
  std::string upperValue = toUpper(value);

  static const std::map<std::string, cfg::QueueScheduling> shortNames = {
      {"WRR", cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN},
      {"SP", cfg::QueueScheduling::STRICT_PRIORITY},
      {"DRR", cfg::QueueScheduling::DEFICIT_ROUND_ROBIN},
  };

  auto it = shortNames.find(upperValue);
  if (it != shortNames.end()) {
    return it->second;
  }

  cfg::QueueScheduling scheduling{};
  if (apache::thrift::TEnumTraits<cfg::QueueScheduling>::findValue(
          upperValue, &scheduling)) {
    return scheduling;
  }
  return std::nullopt;
}

// The AQM argument stream is a flat list of <attribute> <value> pairs; the
// detection/linear nesting only exists in the thrift output, not the input.
// congestion-behavior is applied by the caller (see findCongestionBehavior),
// so here it is accepted and skipped; detection asserts the (only supported)
// linear type, and the rest set fields on the linear detection struct.
void parseAqmAttributes(
    const std::vector<std::string>& aqmArgs,
    cfg::ActiveQueueManagement& aqm) {
  if (aqmArgs.empty()) {
    throw std::invalid_argument(
        "active-queue-management requires sub-attributes: "
        "congestion-behavior <value> or detection linear <attr> <value> ...");
  }

  cfg::LinearQueueCongestionDetection linear;
  if (aqm.detection().has_value() && aqm.detection()->linear().has_value()) {
    linear = *aqm.detection()->linear();
  }
  bool sawDetectionArgs = false;

  for (size_t i = 0; i < aqmArgs.size(); i += 2) {
    const auto& attr = aqmArgs[i];
    if (i + 1 >= aqmArgs.size()) {
      throw std::invalid_argument(fmt::format("'{}' requires a value", attr));
    }
    const auto& value = aqmArgs[i + 1];

    if (attr == "congestion-behavior") {
      // Parsed and applied by the caller (findCongestionBehavior +
      // selectOrCreateAqm); accept the token here without re-parsing it.
    } else if (attr == "detection") {
      if (toUpper(value) != "LINEAR") {
        throw std::invalid_argument(
            fmt::format(
                "Invalid detection type: '{}'. Currently supported: linear",
                value));
      }
      sawDetectionArgs = true;
    } else if (auto it = linearSetters().find(attr);
               it != linearSetters().end()) {
      it->second(linear, parseNonNegativeInt(attr, value));
      sawDetectionArgs = true;
    } else {
      throw std::invalid_argument(
          fmt::format(
              "Unknown active-queue-management attribute: '{}'. Valid "
              "attributes are: congestion-behavior, detection, {}",
              attr,
              getValidLinearAttrs()));
    }
  }

  // Only touch detection when the user actually supplied detection/linear
  // args; otherwise a bare `congestion-behavior <x>` would clobber an existing
  // detection with an empty linear struct.
  if (sawDetectionArgs) {
    aqm.detection() = cfg::QueueCongestionDetection();
    aqm.detection()->linear() = linear;
  }
}

// Parses the congestion-behavior named in the AQM arg stream (the key the
// caller uses to target the matching AQM list entry). Returns nullopt when the
// user did not name one. This is the single place the behavior is parsed, so it
// also rejects a second, possibly conflicting, congestion-behavior token.
std::optional<cfg::QueueCongestionBehavior> findCongestionBehavior(
    const std::vector<std::string>& aqmArgs) {
  std::optional<cfg::QueueCongestionBehavior> behavior;
  for (size_t i = 0; i + 1 < aqmArgs.size(); i += 2) {
    if (aqmArgs[i] == "congestion-behavior") {
      if (behavior.has_value()) {
        throw std::invalid_argument(
            "congestion-behavior may only be specified once");
      }
      behavior = parseThriftEnum<cfg::QueueCongestionBehavior>(
          aqmArgs[i], aqmArgs[i + 1]);
    }
  }
  return behavior;
}

// Picks the AQM list entry a config edit should modify, keyed by behavior. A
// port queue may carry two AQM entries (one ECN, one EARLY_DROP); selecting by
// behavior lets them coexist instead of clobbering aqms[0]. Returns the
// existing entry for this behavior, or appends and returns a new one.
cfg::ActiveQueueManagement& selectOrCreateAqm(
    cfg::PortQueue& queue,
    cfg::QueueCongestionBehavior behavior) {
  auto& aqms = queue.aqms().ensure();
  for (auto& aqm : aqms) {
    if (aqm.behavior().has_value() && *aqm.behavior() == behavior) {
      return aqm;
    }
  }
  return aqms.emplace_back();
}

} // namespace

void applyPortQueueConfig(
    cfg::PortQueue& queue,
    const std::vector<std::pair<std::string, std::string>>& attributes,
    const std::vector<std::string>& aqmArgs) {
  if (attributes.empty() && aqmArgs.empty()) {
    throw std::invalid_argument(
        "At least one attribute is required: " + validTopLevelAttrs());
  }

  for (const auto& [attr, value] : attributes) {
    if (attr == "reserved-bytes") {
      queue.reservedBytes() = parseNonNegativeInt(attr, value);
    } else if (attr == "shared-bytes") {
      queue.sharedBytes() = parseNonNegativeInt(attr, value);
    } else if (attr == "weight") {
      queue.weight() = parseNonNegativeInt(attr, value);
    } else if (attr == "scaling-factor") {
      queue.scalingFactor() =
          parseThriftEnum<cfg::MMUScalingFactor>(attr, value);
    } else if (attr == "scheduling") {
      auto scheduling = parseScheduling(value);
      if (!scheduling) {
        throw std::invalid_argument(
            "Invalid scheduling: '" + value +
            "'. Valid values are: " + getValidSchedulingTypes());
      }
      queue.scheduling() = *scheduling;
    } else if (attr == "stream-type") {
      queue.streamType() = parseThriftEnum<cfg::StreamType>(attr, value);
    } else if (attr == "buffer-pool-name") {
      queue.bufferPoolName() = value;
    } else {
      throw std::invalid_argument(
          "Unknown attribute: '" + attr +
          "'. Valid attributes are: " + validTopLevelAttrs());
    }
  }

  if (!aqmArgs.empty()) {
    // congestion-behavior is the key that identifies which AQM policy on the
    // queue this edit targets. Without it the target is ambiguous (a queue can
    // hold both an ECN and an EARLY_DROP entry) and creating an entry would
    // silently commit behavior=EARLY_DROP (thrift enum default 0). Require it.
    auto behavior = findCongestionBehavior(aqmArgs);
    if (!behavior.has_value()) {
      throw std::invalid_argument(
          "active-queue-management requires 'congestion-behavior "
          "<ECN|EARLY_DROP>' to identify which AQM policy to edit");
    }
    auto& aqm = selectOrCreateAqm(queue, *behavior);
    aqm.behavior() = *behavior;
    parseAqmAttributes(aqmArgs, aqm);
  }
}

} // namespace facebook::fboss::utils
