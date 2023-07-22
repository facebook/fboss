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

#include "fboss/agent/gen-cpp2/agent_config_types.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <memory>

namespace facebook::fboss {

struct AgentConfig {
  explicit AgentConfig(const cfg::AgentConfig& thriftConfig);
  AgentConfig(cfg::AgentConfig thriftConfig, std::string rawConfig)
      : thrift(std::move(thriftConfig)), raw(std::move(rawConfig)) {}

  // creators
  static std::unique_ptr<AgentConfig> fromDefaultFile();
  static std::unique_ptr<AgentConfig> fromFile(folly::StringPiece path);
  static std::unique_ptr<AgentConfig> fromRawConfig(
      const std::string& contents);

  // serialize just the sw component
  std::string swConfigRaw() const;
  // serialize entire agent config
  std::string agentConfigRaw() const;

  void dumpConfig(folly::StringPiece path) const;

  const cfg::AgentConfig thrift;
  const std::string raw;
};

std::unique_ptr<facebook::fboss::AgentConfig> createEmptyAgentConfig();
} // namespace facebook::fboss
