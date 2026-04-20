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
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/acl/CmdConfigAcl.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

// Argument for `config acl table-group <group-name> stage <stage-value>`.
//
// <group-name> is the name of an existing AclTableGroup in sw.aclTableGroups.
// <stage-value> is one of: ingress, ingress-macsec, egress-macsec,
//   ingress-post-lookup (or a numeric 0-3).
class AclTableGroupConfigArgs : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ AclTableGroupConfigArgs( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  const std::string& getGroupName() const {
    return groupName_;
  }

  cfg::AclStage getStage() const {
    return stage_;
  }

 private:
  std::string groupName_;
  cfg::AclStage stage_{cfg::AclStage::INGRESS};
};

struct CmdConfigAclTableGroupTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigAcl;
  using ObjectArgType = AclTableGroupConfigArgs;
  using RetType = std::string;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    // required() + expected(3) prevents CLI11 from treating the first
    // positional (the group name) as a subcommand name.
    cmd.add_option(
           "acl_table_group_config",
           args,
           "<group-name> stage <stage-value> where <stage-value> is one "
           "of: ingress, ingress-macsec, egress-macsec, "
           "ingress-post-lookup (or numeric 0-3)")
        ->required()
        ->expected(3);
  }
};

class CmdConfigAclTableGroup
    : public CmdHandler<CmdConfigAclTableGroup, CmdConfigAclTableGroupTraits> {
 public:
  using ObjectArgType = CmdConfigAclTableGroupTraits::ObjectArgType;
  using RetType = CmdConfigAclTableGroupTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& args);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
