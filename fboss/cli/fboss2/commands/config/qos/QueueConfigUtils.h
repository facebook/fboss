/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <string>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

namespace facebook::fboss::utils {

// Reserved queue-config name designating the switch-wide default queue list.
// `queue-config default` edits SwitchConfig::defaultPortQueues -- the fallback
// every port without an explicit portQueueConfigName uses -- while any other
// name edits a SwitchConfig::portQueueConfigs entry. Reserving the name is what
// keeps the two disjoint: portQueueConfigs can never acquire a "default" key,
// so Port::portQueueConfigName can never resolve to the default list.
inline constexpr auto kDefaultQueueConfigName = "default";

/**
 * Name of a queue config: either a portQueueConfigs key or the reserved
 * `default`.
 */
class QueueConfigName : public BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ QueueConfigName(std::vector<std::string> v);

  const std::string& getName() const {
    return data_[0];
  }

  bool isDefault() const {
    return data_[0] == kDefaultQueueConfigName;
  }
};

// Resolves `name` to the PortQueue list it designates, creating an empty
// portQueueConfigs entry if a named config does not exist yet.
std::vector<cfg::PortQueue>& queueConfigListForWrite(
    cfg::SwitchConfig& switchConfig,
    const QueueConfigName& name);

// Resolves `name` to the PortQueue list it designates, or nullptr if a named
// config does not exist. Callers that must not create an entry on a typo --
// delete, in particular -- use this rather than queueConfigListForWrite.
// `default` always resolves, since defaultPortQueues always exists.
std::vector<cfg::PortQueue>* findQueueConfigList(
    cfg::SwitchConfig& switchConfig,
    const QueueConfigName& name);

// Names of the ports whose Port::portQueueConfigName references `name`, so a
// caller can refuse to remove a queue config that is still bound and would
// leave those ports pointing at nothing.
//
// Always empty for the reserved `default`: no port names it explicitly, and
// every port without a binding falls back to it, so it can never be dangling.
std::vector<std::string> portsUsingQueueConfig(
    const cfg::SwitchConfig& switchConfig,
    const QueueConfigName& name);

} // namespace facebook::fboss::utils
