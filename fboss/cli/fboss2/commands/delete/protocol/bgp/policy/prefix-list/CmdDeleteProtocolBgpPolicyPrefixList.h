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
#include "fboss/cli/fboss2/commands/delete/protocol/bgp/policy/CmdDeleteProtocolBgpPolicy.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

// Parsed `delete protocol bgp policy prefix-list <name>`, validated at
// construction. The list name is the only identity the config command
// stores at this level; deleting a single entry arrives with the entry
// subcommand (CmdConfigProtocolBgpPolicyPrefixListEntry), where the entry's
// identity is decided.
class BgpPrefixListRef : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ BgpPrefixListRef(std::vector<std::string> v);
  const std::string& listName() const {
    return listName_;
  }
  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_MESSAGE;

 private:
  std::string listName_;
};

struct CmdDeleteProtocolBgpPolicyPrefixListTraits : public WriteCommandTraits {
  using ParentCmd = CmdDeleteProtocolBgpPolicy;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option("args", args, "<name>");
  }
  using ObjectArgType = BgpPrefixListRef;
  using RetType = std::string;
};

class CmdDeleteProtocolBgpPolicyPrefixList
    : public CmdHandler<
          CmdDeleteProtocolBgpPolicyPrefixList,
          CmdDeleteProtocolBgpPolicyPrefixListTraits> {
 public:
  using ObjectArgType =
      CmdDeleteProtocolBgpPolicyPrefixListTraits::ObjectArgType;
  using RetType = CmdDeleteProtocolBgpPolicyPrefixListTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& args);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
