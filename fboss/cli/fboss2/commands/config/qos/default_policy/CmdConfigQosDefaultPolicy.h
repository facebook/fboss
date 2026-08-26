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
#include "fboss/cli/fboss2/commands/config/qos/CmdConfigQos.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

class DefaultPolicyName : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ DefaultPolicyName(std::vector<std::string> v);

  const std::string& getName() const {
    return data_[0];
  }
};

struct CmdConfigQosDefaultPolicyTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigQos;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option("policy_name", args, "QoS policy name to set as default");
  }
  using ObjectArgType = DefaultPolicyName;
  using RetType = std::string;
};

class CmdConfigQosDefaultPolicy : public CmdHandler<
                                      CmdConfigQosDefaultPolicy,
                                      CmdConfigQosDefaultPolicyTraits> {
 public:
  using ObjectArgType = CmdConfigQosDefaultPolicyTraits::ObjectArgType;
  using RetType = CmdConfigQosDefaultPolicyTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& policyName);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
