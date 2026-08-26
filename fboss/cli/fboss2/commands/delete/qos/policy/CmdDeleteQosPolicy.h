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
#include "fboss/cli/fboss2/commands/config/qos/policy/CmdConfigQosPolicy.h"
#include "fboss/cli/fboss2/commands/delete/qos/CmdDeleteQos.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

struct CmdDeleteQosPolicyTraits : public WriteCommandTraits {
  using ParentCmd = CmdDeleteQos;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    // required() + expected(1) keeps CLI11 from reclassifying the policy name
    // as a subcommand when it happens to match one elsewhere in the tree.
    cmd.add_option(
           "qos_policy_name",
           args,
           "Name of the QoS policy to remove from qosPolicies")
        ->required()
        ->expected(1);
  }
  // Shared with `config qos policy` so the accepted name syntax cannot drift
  // between the two commands.
  using ObjectArgType = QosPolicyName;
  using RetType = std::string;
};

class CmdDeleteQosPolicy
    : public CmdHandler<CmdDeleteQosPolicy, CmdDeleteQosPolicyTraits> {
 public:
  using ObjectArgType = CmdDeleteQosPolicyTraits::ObjectArgType;
  using RetType = CmdDeleteQosPolicyTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& policyName);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
