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
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/delete/acl/CmdDeleteAcl.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

// Argument for `delete acl rule <table-name> <rule-name>`.
//
// <table-name> matches an AclTable inside any AclTableGroup in
//   sw.aclTableGroups (field 56). The deprecated aclTableGroup (field 45)
//   is not supported.
// <rule-name>  matches an AclEntry inside that table by name. The entry
//   is removed from the table.
class AclRuleDeleteArgs : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ AclRuleDeleteArgs( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  const std::string& getTableName() const {
    return tableName_;
  }

  const std::string& getRuleName() const {
    return ruleName_;
  }

 private:
  std::string tableName_;
  std::string ruleName_;
};

struct CmdDeleteAclRuleTraits : public WriteCommandTraits {
  using ParentCmd = CmdDeleteAcl;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
           "acl_rule_delete",
           args,
           "<table-name> <rule-name>: removes the AclEntry from the table")
        ->required()
        ->expected(2);
  }
  using ObjectArgType = AclRuleDeleteArgs;
  using RetType = std::string;
};

class CmdDeleteAclRule
    : public CmdHandler<CmdDeleteAclRule, CmdDeleteAclRuleTraits> {
 public:
  using ObjectArgType = CmdDeleteAclRuleTraits::ObjectArgType;
  using RetType = CmdDeleteAclRuleTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& args);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
