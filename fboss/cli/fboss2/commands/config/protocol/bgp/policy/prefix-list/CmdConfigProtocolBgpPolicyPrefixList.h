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

// Parsed `prefix-list <name> [entry <seq-num>] [<attribute> <value> ...]`,
// validated at construction. A PrefixList is keyed by <name>; within a list,
// `entry <seq-num>` selects (or creates) a seq_num-keyed PrefixListEntry in
// prefixes[]. Attributes apply to the list (bare `<name> <attr>`) or the
// entry (`<name> entry <seq> <attr>`) depending on whether an entry key is
// present. Mirrors BgpAsPathListConfig.
//
// Grammar (from the FBOSS proposed syntax):
//   prefix-list <name>                                    (create/select list)
//   prefix-list <name> boolean-operator <op>              (list attribute)
//   prefix-list <name> compare-operator <op>              (list attribute)
//   prefix-list <name> description <string>               (list attribute)
//   prefix-list <name> ip-version <v4|v6>                 (list attribute)
//   prefix-list <name> entry <seq-num>                   (create/select entry)
//   prefix-list <name> entry <seq-num> base-prefix <prefix/len>
//   prefix-list <name> entry <seq-num> communities <community-string>
//   prefix-list <name> entry <seq-num> description <string>
//   prefix-list <name> entry <seq-num> match-logic <EQUAL|NOT_EQUAL>
//   prefix-list <name> entry <seq-num> max-allowed-subnet-count <value>
//   prefix-list <name> entry <seq-num> prefix-len-range compare-operator <op>
//   prefix-list <name> entry <seq-num> prefix-len-range value <0-128>
//   prefix-list <name> entry <seq-num> regex <string>
class BgpPrefixListConfig : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ BgpPrefixListConfig(std::vector<std::string> v);
  const std::string& listName() const {
    return listName_;
  }
  bool hasEntry() const {
    return hasEntry_;
  }
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
  std::string listName_;
  bool hasEntry_{false};
  int32_t seqNum_{0};
  std::string attr_; // matched dispatch key ("" = bare create)
  std::vector<std::string> values_;
};

// Single handler for the whole `config protocol bgp policy prefix-list`
// family; dispatch and per-attribute parsing live in the .cpp so a new tunable
// is a one-entry change. Mirrors CmdConfigProtocolBgpPolicyAsPathList.
struct CmdConfigProtocolBgpPolicyPrefixListTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigProtocolBgpPolicy;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    // Stops CLI11 from classifying attribute tokens as subcommands once the
    // list name is consumed. See CmdConfigProtocolBgpNeighborTraits.
    cmd.positionals_at_end();
    cmd.add_option(
        "args", args, "<name> [entry <seq-num>] [<attribute> <value> ...]");
  }
  using ObjectArgType = BgpPrefixListConfig;
  using RetType = std::string;
};

class CmdConfigProtocolBgpPolicyPrefixList
    : public CmdHandler<
          CmdConfigProtocolBgpPolicyPrefixList,
          CmdConfigProtocolBgpPolicyPrefixListTraits> {
 public:
  using ObjectArgType =
      CmdConfigProtocolBgpPolicyPrefixListTraits::ObjectArgType;
  using RetType = CmdConfigProtocolBgpPolicyPrefixListTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& args);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
