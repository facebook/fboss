/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/qos/QueueConfigUtils.h"

#include <fmt/format.h>
#include <folly/String.h>
#include <re2/re2.h>
#include <stdexcept>

namespace facebook::fboss::utils {

QueueConfigName::QueueConfigName(std::vector<std::string> v) {
  if (v.empty()) {
    throw std::invalid_argument("Queue config name is required");
  }
  if (v.size() != 1) {
    throw std::invalid_argument(
        "Expected a single queue config name, got: " + folly::join(", ", v));
  }
  const auto& name = v[0];
  // Starts with a letter, then alphanumerics/underscore/hyphen, 1-64 chars.
  // The reserved name `default` satisfies this, so it needs no special case
  // here; the commands distinguish it via isDefault().
  static const re2::RE2 kValidNamePattern("^[a-zA-Z][a-zA-Z0-9_-]{0,63}$");
  if (!re2::RE2::FullMatch(name, kValidNamePattern)) {
    throw std::invalid_argument(
        "Invalid queue config name: '" + name +
        "'. Name must start with a letter, contain only alphanumeric "
        "characters, underscores, or hyphens, and be 1-64 characters long.");
  }
  data_.push_back(name);
}

std::vector<cfg::PortQueue>& queueConfigListForWrite(
    cfg::SwitchConfig& switchConfig,
    const QueueConfigName& name) {
  if (name.isDefault()) {
    return *switchConfig.defaultPortQueues();
  }
  return (*switchConfig.portQueueConfigs())[name.getName()];
}

std::vector<cfg::PortQueue>* findQueueConfigList(
    cfg::SwitchConfig& switchConfig,
    const QueueConfigName& name) {
  if (name.isDefault()) {
    return &*switchConfig.defaultPortQueues();
  }
  auto& configs = *switchConfig.portQueueConfigs();
  auto it = configs.find(name.getName());
  return it == configs.end() ? nullptr : &it->second;
}

std::vector<std::string> portsUsingQueueConfig(
    const cfg::SwitchConfig& switchConfig,
    const QueueConfigName& name) {
  std::vector<std::string> portNames;
  if (name.isDefault()) {
    return portNames;
  }
  for (const auto& port : *switchConfig.ports()) {
    if (port.portQueueConfigName().has_value() &&
        *port.portQueueConfigName() == name.getName()) {
      // Port::name is optional; switch_config.thrift documents the fallback as
      // "port-<logicalID>", so use that rather than reporting an empty name.
      if (port.name().has_value()) {
        portNames.push_back(*port.name());
      } else {
        portNames.push_back(fmt::format("port-{}", *port.logicalID()));
      }
    }
  }
  return portNames;
}

} // namespace facebook::fboss::utils
