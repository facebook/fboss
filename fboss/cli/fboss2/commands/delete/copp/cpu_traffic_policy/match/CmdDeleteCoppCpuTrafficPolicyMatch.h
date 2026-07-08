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

#include <fmt/format.h>
#include <stdexcept>
#include <string>
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/delete/copp/cpu_traffic_policy/CmdDeleteCoppCpuTrafficPolicy.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

namespace facebook::fboss {

class CoppMatcherName : public utils::BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ CoppMatcherName(std::vector<std::string> v) {
    if (v.empty()) {
      throw std::invalid_argument("matcher-name is required");
    }
    if (v.size() != 1) {
      throw std::invalid_argument(
          fmt::format("Expected exactly one matcher-name, got {}", v.size()));
    }
    if (v[0].empty()) {
      throw std::invalid_argument("matcher-name must not be empty");
    }
    data_ = std::move(v);
  }

  const std::string& getName() const {
    return data_[0];
  }
};

struct CmdDeleteCoppCpuTrafficPolicyMatchTraits : public WriteCommandTraits {
  using ParentCmd = CmdDeleteCoppCpuTrafficPolicy;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
           "matcher_name",
           args,
           "Name of the ACL matcher entry to target in "
           "cpuTrafficPolicy.trafficPolicy.matchToAction")
        ->required()
        ->expected(1);
  }
  using ObjectArgType = CoppMatcherName;
  using RetType = std::string;
};

class CmdDeleteCoppCpuTrafficPolicyMatch
    : public CmdHandler<
          CmdDeleteCoppCpuTrafficPolicyMatch,
          CmdDeleteCoppCpuTrafficPolicyMatchTraits> {
 public:
  using ObjectArgType = CmdDeleteCoppCpuTrafficPolicyMatchTraits::ObjectArgType;
  using RetType = CmdDeleteCoppCpuTrafficPolicyMatchTraits::RetType;

  RetType queryClient(
      const HostInfo& /* hostInfo */,
      const ObjectArgType& /* matcherName */) {
    throw std::runtime_error(
        "Incomplete command, please use one of the subcommands");
  }

  void printOutput(const RetType& /* model */) {}
};

} // namespace facebook::fboss
