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
#include "CLI/App.hpp"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/CmdConfigProtocolBgpPolicy.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

// Parsed `as-path-list <name> [<attribute> <value> ...]`, validated at
// construction. An AsPathList is keyed by <name>. The list's entries are their
// own subcommand (CmdConfigProtocolBgpPolicyAsPathListEntry), not attributes.
//
// Grammar (from the FBOSS proposed syntax):
//   as-path-list <name>                          (create/select list)
//   as-path-list <name> description <string>     (list attribute)
class BgpAsPathListConfig : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ BgpAsPathListConfig(std::vector<std::string> v);
  const std::string& listName() const {
    return listName_;
  }
  const std::string& attr() const {
    return attr_;
  }
  const std::vector<std::string>& values() const {
    return values_;
  }
  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_MESSAGE;

 private:
  std::string listName_;
  std::string attr_; // matched dispatch key ("" = bare create)
  std::vector<std::string> values_;
};

// The list level of the `config protocol bgp policy as-path-list` family;
// dispatch and per-attribute parsing live in the .cpp so a new tunable is a
// one-entry change. Mirrors CmdConfigProtocolBgpPolicyRoutingPolicy, whose
// nested level (term / entry) is likewise its own subcommand.
struct CmdConfigProtocolBgpPolicyAsPathListTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigProtocolBgpPolicy;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    // No positionals_at_end() here: CLI11 must stay free to classify the
    // `entry` token as this command's subcommand rather than swallowing it
    // into args. See CmdConfigProtocolBgpPolicyRoutingPolicyTraits.
    cmd.add_option("args", args, "<name> [<attribute> <value> ...]");
  }
  using ObjectArgType = BgpAsPathListConfig;
  using RetType = std::string;
};

class CmdConfigProtocolBgpPolicyAsPathList
    : public CmdHandler<
          CmdConfigProtocolBgpPolicyAsPathList,
          CmdConfigProtocolBgpPolicyAsPathListTraits> {
 public:
  using ObjectArgType =
      CmdConfigProtocolBgpPolicyAsPathListTraits::ObjectArgType;
  using RetType = CmdConfigProtocolBgpPolicyAsPathListTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& args);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
