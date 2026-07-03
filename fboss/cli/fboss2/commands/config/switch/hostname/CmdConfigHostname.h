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
#include "fboss/cli/fboss2/commands/config/switch/CmdConfigSwitch.h"

namespace facebook::fboss {

// Args: <name> — a non-empty string hostname (RFC 952/1123 compliant)
class HostnameConfigArgs : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ HostnameConfigArgs( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  const std::string& getName() const {
    return data_[0];
  }
};

struct CmdConfigHostnameTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigSwitch;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option("hostname", args, "Switch hostname")
        ->required()
        ->expected(1);
  }
  using ObjectArgType = HostnameConfigArgs;
  using RetType = std::string;
};

class CmdConfigHostname
    : public CmdHandler<CmdConfigHostname, CmdConfigHostnameTraits> {
 public:
  using ObjectArgType = CmdConfigHostnameTraits::ObjectArgType;
  using RetType = CmdConfigHostnameTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& args);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
