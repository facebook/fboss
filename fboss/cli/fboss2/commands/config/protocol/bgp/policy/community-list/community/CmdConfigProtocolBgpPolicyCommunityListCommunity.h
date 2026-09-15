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
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/community-list/CmdConfigProtocolBgpPolicyCommunityList.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

// Parsed `community <name> [<attribute> <value> ...]`, validated at
// construction. A member (bgp_policy.Community, held inline through the
// CommunityRefType union's `community` arm in CommunityList.members[]) is
// keyed by <name>; the list it belongs to is supplied by the parent command's
// args.
//
// Grammar (from the FBOSS proposed syntax):
//   ... community-list <name> community <name>                (create/select)
//   ... community-list <name> community <name> description <string>
//   ... community-list <name> community <name> type <type>
//   ... community-list <name> community <name> value <string>
class BgpCommunityListCommunityConfig
    : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ BgpCommunityListCommunityConfig(std::vector<std::string> v);
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
  std::string communityName_;
  std::string attr_; // matched dispatch key ("" = bare create)
  std::vector<std::string> values_;
};

// The community (member) level of the community-list family as its own CLI11
// subcommand; the parent's parsed args arrive through the ancestor-args tuple,
// mirroring CmdConfigProtocolBgpPolicyAsPathListEntry.
struct CmdConfigProtocolBgpPolicyCommunityListCommunityTraits
    : public WriteCommandTraits {
  using ParentCmd = CmdConfigProtocolBgpPolicyCommunityList;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    // Community has no nested subcommands; stop CLI11's parent-chain
    // fallthrough from stealing a value token that spells `community`.
    cmd.positionals_at_end();
    cmd.add_option("args", args, "<name> [<attribute> <value> ...]");
  }
  using ObjectArgType = BgpCommunityListCommunityConfig;
  using RetType = std::string;
};

class CmdConfigProtocolBgpPolicyCommunityListCommunity
    : public CmdHandler<
          CmdConfigProtocolBgpPolicyCommunityListCommunity,
          CmdConfigProtocolBgpPolicyCommunityListCommunityTraits> {
 public:
  using ObjectArgType =
      CmdConfigProtocolBgpPolicyCommunityListCommunityTraits::ObjectArgType;
  using RetType =
      CmdConfigProtocolBgpPolicyCommunityListCommunityTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const BgpCommunityListConfig& listArgs,
      const ObjectArgType& args);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
