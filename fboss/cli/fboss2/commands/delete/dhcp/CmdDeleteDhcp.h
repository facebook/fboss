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
#include <string_view>
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

namespace facebook::fboss::cfg {
class SwitchConfig;
} // namespace facebook::fboss::cfg

namespace facebook::fboss {

// Argument type for `delete dhcp {relay,reply}-source-override <family>`.
// Validates family ∈ {"ipv4","ipv6"} via normalizeDhcpFamily.
class DhcpSourceOverrideDeleteArgs
    : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */
  DhcpSourceOverrideDeleteArgs( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  const std::string& getFamily() const {
    return family_;
  }

 private:
  std::string family_;
};

// Unsets the optional source-override field for (kind, family) and returns
// the human-readable result string. Throws std::invalid_argument if the
// field is not currently set. kind must be "relay" or "reply".
std::string removeDhcpSourceOverride(
    cfg::SwitchConfig& swConfig,
    std::string_view kind,
    const std::string& family);

struct CmdDeleteDhcpTraits : public WriteCommandTraits {
  using ObjectArgType = utils::NoneArgType;
  using RetType = std::string;
};

class CmdDeleteDhcp : public CmdHandler<CmdDeleteDhcp, CmdDeleteDhcpTraits> {
 public:
  using ObjectArgType = CmdDeleteDhcpTraits::ObjectArgType;
  using RetType = CmdDeleteDhcpTraits::RetType;

  RetType queryClient(const HostInfo& /* hostInfo */) {
    throw std::runtime_error(
        "Incomplete command, please use 'relay-source-override' or "
        "'reply-source-override' subcommand");
  }

  void printOutput(const RetType& /* model */) {}
};

} // namespace facebook::fboss
