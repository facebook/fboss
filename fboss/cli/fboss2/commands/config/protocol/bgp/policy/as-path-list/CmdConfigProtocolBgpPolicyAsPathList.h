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

#include <cstdint>
#include <string>
#include <vector>
#include "CLI/App.hpp"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/CmdConfigProtocolBgpPolicy.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

// Parsed `as-path-list <name> [entry <seq-num>] [<attribute> <value> ...]`,
// validated at construction. An AsPathList is keyed by <name>; within a list,
// `entry <seq-num>` selects (or creates) a keyed AsPathListEntry. Attributes
// apply to the list (bare `<name> <attr>`) or the entry (`<name> entry <seq>
// <attr>`) depending on whether an entry key is present.
//
// Grammar (from the FBOSS proposed syntax):
//   as-path-list <name>                                  (create/select list)
//   as-path-list <name> description <string>             (list attribute)
//   as-path-list <name> entry <seq-num>                  (create/select entry)
//   as-path-list <name> entry <seq-num> asn-regexp <re>  (entry attribute)
//   as-path-list <name> entry <seq-num> description <s>  (entry attribute)
//   as-path-list <name> entry <seq-num> match-logic <op> (entry attribute)
class BgpAsPathListConfig : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ BgpAsPathListConfig(std::vector<std::string> v);
  const std::string& listName() const {
    return listName_;
  }
  bool hasEntry() const {
    return hasEntry_;
  }
  int64_t seqNum() const {
    return seqNum_;
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
  bool hasEntry_{false};
  int64_t seqNum_{0};
  std::string attr_; // matched dispatch key ("" = bare create)
  std::vector<std::string> values_;
};

// Single handler for the whole `config protocol bgp policy as-path-list`
// family; dispatch and per-attribute parsing live in the .cpp so a new tunable
// is a one-entry change. Mirrors CmdConfigProtocolBgpPeerGroup, with a second
// (entry) key level.
struct CmdConfigProtocolBgpPolicyAsPathListTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigProtocolBgpPolicy;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    // Stops CLI11 from classifying attribute tokens as subcommands once the
    // list name is consumed. See CmdConfigProtocolBgpNeighborTraits.
    cmd.positionals_at_end();
    cmd.add_option(
        "args", args, "<name> [entry <seq-num>] [<attribute> <value> ...]");
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
