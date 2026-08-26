/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/traffic_counter/CmdConfigTrafficCounter.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/String.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <algorithm>
#include <iostream>
#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

namespace {
// Every cfg::CounterType spelling the CLI accepts, matched case insensitively.
std::string validTypeList() {
  std::vector<std::string> names;
  for (auto type : apache::thrift::TEnumTraits<cfg::CounterType>::values) {
    names.push_back(apache::thrift::util::enumNameSafe(type));
  }
  return folly::join(",", names);
}

std::string typesToString(const std::vector<cfg::CounterType>& types) {
  std::vector<std::string> names;
  names.reserve(types.size());
  for (auto type : types) {
    names.push_back(apache::thrift::util::enumNameSafe(type));
  }
  return folly::join(",", names);
}
} // namespace

TrafficCounterArg::TrafficCounterArg(std::vector<std::string> v) {
  if (v.size() != 2) {
    throw std::invalid_argument(
        fmt::format(
            "Expected <name> <types>, where types is a comma separated list of {}",
            validTypeList()));
  }

  name_ = v[0];
  if (name_.empty()) {
    throw std::invalid_argument("Counter name must not be empty");
  }
  data_.push_back(v[0]);

  std::vector<std::string> tokens;
  folly::split(',', v[1], tokens);
  for (auto& token : tokens) {
    folly::toUpperAscii(token);
    cfg::CounterType type{};
    if (!apache::thrift::TEnumTraits<cfg::CounterType>::findValue(
            token, &type)) {
      throw std::invalid_argument(
          fmt::format(
              "Invalid counter type '{}'. Expected a comma separated list of {}",
              token,
              validTypeList()));
    }
    if (std::find(types_.begin(), types_.end(), type) == types_.end()) {
      types_.push_back(type);
    }
  }
  // Store in enum order so the config, and the already-configured comparison
  // below, do not depend on the order the types were typed in.
  std::sort(types_.begin(), types_.end());
  data_.push_back(v[1]);
}

CmdConfigTrafficCounterTraits::RetType CmdConfigTrafficCounter::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& trafficCounter) {
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& swConfig = *config.sw();

  const auto& name = trafficCounter.getName();
  const auto& newTypes = trafficCounter.getTypes();

  auto& counters = *swConfig.trafficCounters();
  auto it = std::find_if(
      counters.begin(), counters.end(), [&](const cfg::TrafficCounter& c) {
        return *c.name() == name;
      });

  if (it != counters.end()) {
    // The CLI stores types in enum order, but a hand-written config need not.
    auto existingTypes = *it->types();
    std::sort(existingTypes.begin(), existingTypes.end());
    if (existingTypes == newTypes) {
      return fmt::format(
          "Traffic counter '{}' is already configured with type '{}'",
          name,
          typesToString(newTypes));
    }
    *it->types() = newTypes;
    session.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
    return fmt::format(
        "Successfully updated traffic counter '{}' to type '{}'",
        name,
        typesToString(newTypes));
  }

  cfg::TrafficCounter newCounter;
  newCounter.name() = name;
  newCounter.types() = newTypes;
  counters.push_back(std::move(newCounter));

  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  return fmt::format(
      "Successfully created traffic counter '{}' with type '{}'",
      name,
      typesToString(newTypes));
}

void CmdConfigTrafficCounter::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

// Explicit template instantiation
template void
CmdHandler<CmdConfigTrafficCounter, CmdConfigTrafficCounterTraits>::run();

} // namespace facebook::fboss
