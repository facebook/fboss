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

// Parsed `delete protocol bgp policy community-list <name>
// [community <name>]`, validated at construction. The names are the list's
// and member's identity, matched exactly the same way the config command
// stores them. Without the `community` selector the whole list is deleted;
// with it, only that inline member. Mirrors BgpAsPathListRef.
class BgpCommunityListRef : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ BgpCommunityListRef(std::vector<std::string> v);
  const std::string& listName() const {
    return listName_;
  }
  bool hasCommunity() const {
    return hasCommunity_;
  }
  const std::string& communityName() const {
    return communityName_;
  }
  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_MESSAGE;

 private:
  std::string listName_;
  bool hasCommunity_{false};
  std::string communityName_;
};

struct CmdDeleteProtocolBgpPolicyCommunityListTraits
    : public WriteCommandTraits {
  using ParentCmd = CmdDeleteProtocolBgpPolicy;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option("args", args, "<name> [community <name>]");
  }
  using ObjectArgType = BgpCommunityListRef;
  using RetType = std::string;
};

class CmdDeleteProtocolBgpPolicyCommunityList
    : public CmdHandler<
          CmdDeleteProtocolBgpPolicyCommunityList,
          CmdDeleteProtocolBgpPolicyCommunityListTraits> {
 public:
  using ObjectArgType =
      CmdDeleteProtocolBgpPolicyCommunityListTraits::ObjectArgType;
  using RetType = CmdDeleteProtocolBgpPolicyCommunityListTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& args);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
