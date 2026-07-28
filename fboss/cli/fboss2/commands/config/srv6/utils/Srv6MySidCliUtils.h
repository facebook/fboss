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

// Shared helpers and positional-argument parsers for SRv6 MySID config CLIs.
// All commands stage edits via ConfigSession; nothing is applied until
// `config session commit`.
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

constexpr int16_t kMySidFunctionMin = 1;
constexpr int16_t kMySidFunctionMax = 32767;

std::string canonicalLocatorPrefix(const std::string& prefix);

int16_t parseMySidFunctionValue(const std::string& value);

bool parseMySidIsV6(const std::string& value);

bool hasMySidConfig(const cfg::SwitchConfig& swConfig);

cfg::MySidConfig& requireMySidConfig(cfg::SwitchConfig& swConfig);

std::map<int16_t, cfg::MySidEntryConfig>& ensureMySidEntries(
    cfg::MySidConfig& config);

void requireMatchingLocatorPrefix(
    const cfg::MySidConfig& config,
    const std::string& requestedPrefix);

class LocatorPrefixArg : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ LocatorPrefixArg(std::vector<std::string> v);

  const std::string& getPrefix() const {
    return prefix_;
  }

 private:
  std::string prefix_;
};

enum class MySidConfigEntryType { ADJACENCY, NODE, DECAP };

class MySidAddArg : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ MySidAddArg(std::vector<std::string> v);

  int16_t getFunctionValue() const {
    return functionValue_;
  }

  MySidConfigEntryType getType() const {
    return type_;
  }

  std::string getTypeStr() const;

  bool isV6() const {
    return isV6_;
  }

  const std::string& getPortName() const {
    return portName_;
  }

  const std::string& getNodeAddress() const {
    return nodeAddress_;
  }

  cfg::MySidEntryConfig buildEntryConfig() const;

 private:
  int16_t functionValue_{0};
  MySidConfigEntryType type_{MySidConfigEntryType::DECAP};
  bool isV6_{true};
  std::string portName_;
  std::string nodeAddress_;
};

class MySidDeleteEntryArg : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ MySidDeleteEntryArg(std::vector<std::string> v);

  int16_t getFunctionValue() const {
    return functionValue_;
  }

 private:
  int16_t functionValue_{0};
};

} // namespace facebook::fboss
