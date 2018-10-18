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

namespace facebook {
namespace fboss {

struct AgentConfig {
  AgentConfig(cfg::AgentConfig thriftConfig, std::string rawConfig) :
      thrift(std::move(thriftConfig)),
      raw(std::move(rawConfig)) {}

  // creators
  static std::unique_ptr<AgentConfig> fromDefaultFile();
  static std::unique_ptr<AgentConfig> fromFile(folly::StringPiece path);
  static std::unique_ptr<AgentConfig> fromRawConfig(
      const std::string& contents);

  // serialize just the sw component
  const std::string swConfigRaw() const;

  void dumpConfig(folly::StringPiece path) const;

  const cfg::AgentConfig thrift;
  const std::string raw;
};

} // namespace fboss
} // namespace facebook
