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

// Parsed `community-list <name> [community <name>] [<attribute> <value> ...]`,
// validated at construction. A CommunityList is keyed by <name>; within a
// list, `community <name>` selects (or creates) a named inline Community
// member. Attributes apply to the list (bare `<name> <attr>`) or the member
// (`<name> community <name> <attr>`) depending on whether a community key is
// present. Mirrors BgpAsPathListConfig, with a name-keyed second level in
// place of the seq-num-keyed one.
//
// Grammar (from the FBOSS proposed syntax):
//   community-list <name>                                (create/select list)
//   community-list <name> boolean-operator <op>          (list attribute)
//   community-list <name> description <string>           (list attribute)
//   community-list <name> exact-match <true|false>       (list attribute)
//   community-list <name> community <name>               (create/select member)
//   community-list <name> community <name> description <s>   (member attribute)
//   community-list <name> community <name> type <type>       (member attribute)
//   community-list <name> community <name> value <string>    (member attribute)
class BgpCommunityListConfig : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ BgpCommunityListConfig(std::vector<std::string> v);
  const std::string& listName() const {
    return listName_;
  }
  bool hasCommunity() const {
    return hasCommunity_;
  }
  const std::string& communityName() const {
    return communityName_;
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
  bool hasCommunity_{false};
  std::string communityName_;
  std::string attr_; // matched dispatch key ("" = bare create)
  std::vector<std::string> values_;
};

// Single handler for the whole `config protocol bgp policy community-list`
// family; dispatch and per-attribute parsing live in the .cpp so a new tunable
// is a one-entry change. Mirrors CmdConfigProtocolBgpPolicyAsPathList.
struct CmdConfigProtocolBgpPolicyCommunityListTraits
    : public WriteCommandTraits {
  using ParentCmd = CmdConfigProtocolBgpPolicy;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    // Stops CLI11 from classifying attribute tokens as subcommands once the
    // list name is consumed. See CmdConfigProtocolBgpNeighborTraits.
    cmd.positionals_at_end();
    cmd.add_option(
        "args", args, "<name> [community <name>] [<attribute> <value> ...]");
  }
  using ObjectArgType = BgpCommunityListConfig;
  using RetType = std::string;
};

class CmdConfigProtocolBgpPolicyCommunityList
    : public CmdHandler<
          CmdConfigProtocolBgpPolicyCommunityList,
          CmdConfigProtocolBgpPolicyCommunityListTraits> {
 public:
  using ObjectArgType =
      CmdConfigProtocolBgpPolicyCommunityListTraits::ObjectArgType;
  using RetType = CmdConfigProtocolBgpPolicyCommunityListTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& args);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
