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
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/as-path-list/CmdConfigProtocolBgpPolicyAsPathList.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

// Parsed `entry <seq-num> [<attribute> <value> ...]`, validated at
// construction. An entry (bgp_policy.AsPathListEntry in
// AsPathList.as_path_list[]) is keyed by <seq-num>; the list it belongs to is
// supplied by the parent command's args.
//
// Grammar (from the FBOSS proposed syntax):
//   ... as-path-list <name> entry <seq-num>                  (create/select)
//   ... as-path-list <name> entry <seq-num> asn-regexp <re>
//   ... as-path-list <name> entry <seq-num> description <string>
//   ... as-path-list <name> entry <seq-num> match-logic <op>
class BgpAsPathListEntryConfig : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ BgpAsPathListEntryConfig(std::vector<std::string> v);
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
  int64_t seqNum_{0};
  std::string attr_; // matched dispatch key ("" = bare create)
  std::vector<std::string> values_;
};

// The entry level of the as-path-list family as its own CLI11 subcommand; the
// parent's parsed args arrive through the ancestor-args tuple, mirroring
// CmdConfigProtocolBgpPolicyRoutingPolicyTerm.
struct CmdConfigProtocolBgpPolicyAsPathListEntryTraits
    : public WriteCommandTraits {
  using ParentCmd = CmdConfigProtocolBgpPolicyAsPathList;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    // Entry has no nested subcommands; stop CLI11's parent-chain fallthrough
    // from stealing a value token that spells `entry` (e.g. in a description).
    cmd.positionals_at_end();
    cmd.add_option("args", args, "<seq-num> [<attribute> <value> ...]");
  }
  using ObjectArgType = BgpAsPathListEntryConfig;
  using RetType = std::string;
};

class CmdConfigProtocolBgpPolicyAsPathListEntry
    : public CmdHandler<
          CmdConfigProtocolBgpPolicyAsPathListEntry,
          CmdConfigProtocolBgpPolicyAsPathListEntryTraits> {
 public:
  using ObjectArgType =
      CmdConfigProtocolBgpPolicyAsPathListEntryTraits::ObjectArgType;
  using RetType = CmdConfigProtocolBgpPolicyAsPathListEntryTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const BgpAsPathListConfig& listArgs,
      const ObjectArgType& args);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
