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
#include "fboss/cli/fboss2/commands/delete/copp/cpu_traffic_policy/match/CmdDeleteCoppCpuTrafficPolicyMatch.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

namespace facebook::fboss {

class CoppMatchActionArg : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ CoppMatchActionArg(std::vector<std::string> v);

  const std::string& getActionType() const {
    return data_[0];
  }
};

struct CmdDeleteCoppCpuTrafficPolicyMatchActionTraits
    : public WriteCommandTraits {
  using ParentCmd = CmdDeleteCoppCpuTrafficPolicyMatch;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
           "action_type",
           args,
           "CPU-plane action to delete from the matcher entry: "
           "send-to-queue, counter, set-tc, user-defined-trap")
        ->required()
        ->expected(1);
  }
  using ObjectArgType = CoppMatchActionArg;
  using RetType = std::string;
};

class CmdDeleteCoppCpuTrafficPolicyMatchAction
    : public CmdHandler<
          CmdDeleteCoppCpuTrafficPolicyMatchAction,
          CmdDeleteCoppCpuTrafficPolicyMatchActionTraits> {
 public:
  using ObjectArgType =
      CmdDeleteCoppCpuTrafficPolicyMatchActionTraits::ObjectArgType;
  using RetType = CmdDeleteCoppCpuTrafficPolicyMatchActionTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const CoppMatcherName& matcherName,
      const ObjectArgType& actionType);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
