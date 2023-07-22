/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/AgentConfig.h"

#include "fboss/agent/FbossError.h"

#include <folly/FileUtil.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include <iostream>

DEFINE_string(
    config,
    "/etc/coop/agent.conf",
    "The path to the local JSON configuration file");

// NOTE: we use std::cerr because logging libs are likely not
// initialized yet...

namespace facebook::fboss {

std::unique_ptr<AgentConfig> AgentConfig::fromDefaultFile() {
  return fromFile(FLAGS_config);
}

std::unique_ptr<AgentConfig> AgentConfig::fromFile(folly::StringPiece path) {
  std::string configStr;
  if (!folly::readFile(path.data(), configStr)) {
    throw FbossError("unable to read ", path);
  }
  return fromRawConfig(std::move(configStr));
}

std::unique_ptr<AgentConfig> AgentConfig::fromRawConfig(
    const std::string& configStr) {
  cfg::AgentConfig agentConfig;

  apache::thrift::SimpleJSONSerializer::deserialize<cfg::AgentConfig>(
      configStr.c_str(), agentConfig);

  // This is crappy, but because we don't annotate the json with numbers, we
  // have to use the SimpleJSONSerializer, which is extremely permissive about
  // unexpected keys and never throws. To determine if we parsed anything, we
  // check if the sw member has any differences with a default-constructed
  // SwitchConfig. If not, we fall back to treating the config as a
  // SwitchConfig directly.
  if (*agentConfig.sw() == cfg::SwitchConfig()) {
    std::cerr << "Not valid AgentConfig, fallback to parsing as SwitchConfig..."
              << std::endl;
    apache::thrift::SimpleJSONSerializer::deserialize<cfg::SwitchConfig>(
        configStr.c_str(), *agentConfig.sw());
  }

  return std::make_unique<AgentConfig>(std::move(agentConfig), configStr);
}

std::string AgentConfig::swConfigRaw() const {
  return apache::thrift::SimpleJSONSerializer::serialize<std::string>(
      *thrift.sw());
}

std::string AgentConfig::agentConfigRaw() const {
  return apache::thrift::SimpleJSONSerializer::serialize<std::string>(thrift);
}

void AgentConfig::dumpConfig(folly::StringPiece path) const {
  folly::writeFile(raw, path.data());
}

std::unique_ptr<facebook::fboss::AgentConfig> createEmptyAgentConfig() {
  facebook::fboss::cfg::AgentConfig agentCfg;
  return std::make_unique<facebook::fboss::AgentConfig>(
      agentCfg,
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(agentCfg));
}

AgentConfig::AgentConfig(const cfg::AgentConfig& thriftConfig)
    : thrift(thriftConfig),
      raw(apache::thrift::SimpleJSONSerializer::serialize<std::string>(
          thriftConfig)) {}

} // namespace facebook::fboss
