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
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/prefix-list/CmdConfigProtocolBgpPolicyPrefixList.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

// Parsed `entry <seq-num> [<attribute> <value> ...]`, validated at
// construction. An entry (routing_policy.PrefixListEntry in
// PrefixList.prefixes[]) is keyed by <seq-num>; the list it belongs to is
// supplied by the parent command's args.
//
// Grammar (from the FBOSS proposed syntax):
//   ... prefix-list <name> entry <seq-num>                   (create/select)
//   ... prefix-list <name> entry <seq-num> base-prefix <prefix/len>
//   ... prefix-list <name> entry <seq-num> communities <community-string>
//   ... prefix-list <name> entry <seq-num> description <string>
//   ... prefix-list <name> entry <seq-num> match-logic <EQUAL|NOT_EQUAL>
//   ... prefix-list <name> entry <seq-num> max-allowed-subnet-count <value>
//   ... prefix-list <name> entry <seq-num> prefix-len-range compare-operator
//   <op>
//   ... prefix-list <name> entry <seq-num> prefix-len-range value <0-128>
//   ... prefix-list <name> entry <seq-num> regex <string>
class BgpPrefixListEntryConfig : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ BgpPrefixListEntryConfig(std::vector<std::string> v);
  int32_t seqNum() const {
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
  int32_t seqNum_{0};
  std::string attr_; // matched dispatch key ("" = bare create)
  std::vector<std::string> values_;
};

// The entry level of the prefix-list family as its own CLI11 subcommand; the
// parent's parsed args arrive through the ancestor-args tuple, mirroring
// CmdConfigProtocolBgpPolicyAsPathListEntry.
struct CmdConfigProtocolBgpPolicyPrefixListEntryTraits
    : public WriteCommandTraits {
  using ParentCmd = CmdConfigProtocolBgpPolicyPrefixList;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    // Entry has no nested subcommands; stop CLI11's parent-chain fallthrough
    // from stealing a value token that spells `entry` (e.g. in a description).
    cmd.positionals_at_end();
    cmd.add_option("args", args, "<seq-num> [<attribute> <value> ...]");
  }
  using ObjectArgType = BgpPrefixListEntryConfig;
  using RetType = std::string;
};

class CmdConfigProtocolBgpPolicyPrefixListEntry
    : public CmdHandler<
          CmdConfigProtocolBgpPolicyPrefixListEntry,
          CmdConfigProtocolBgpPolicyPrefixListEntryTraits> {
 public:
  using ObjectArgType =
      CmdConfigProtocolBgpPolicyPrefixListEntryTraits::ObjectArgType;
  using RetType = CmdConfigProtocolBgpPolicyPrefixListEntryTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const BgpPrefixListConfig& listArgs,
      const ObjectArgType& args);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
