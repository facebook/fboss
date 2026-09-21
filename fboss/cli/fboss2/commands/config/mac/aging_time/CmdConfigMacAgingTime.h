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

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/mac/CmdConfigMac.h"

namespace facebook::fboss {

class MacAgingTimeArg : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ MacAgingTimeArg( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  int32_t getSeconds() const {
    return seconds_;
  }

 private:
  int32_t seconds_{300};
};

struct CmdConfigMacAgingTimeTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigMac;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option("seconds", args, "MAC aging time in seconds");
  }
  using ObjectArgType = MacAgingTimeArg;
  using RetType = std::string;
};

class CmdConfigMacAgingTime
    : public CmdHandler<CmdConfigMacAgingTime, CmdConfigMacAgingTimeTraits> {
 public:
  using ObjectArgType = CmdConfigMacAgingTimeTraits::ObjectArgType;
  using RetType = CmdConfigMacAgingTimeTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& agingTime);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
