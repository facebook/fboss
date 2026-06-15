// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/set/sdk/CmdSetSdk.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdSetSdkRegDumpTraits : public WriteCommandTraits {
  using ParentCmd = CmdSetSdk;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
        "state",
        args,
        "Whether the SDK dumps register/state logs to disk: "
        "'enable' or 'disable'.");
  }
  using ObjectArgType = utils::Message;
  using RetType = std::string;
};

class CmdSetSdkRegDump
    : public CmdHandler<CmdSetSdkRegDump, CmdSetSdkRegDumpTraits> {
 public:
  using ObjectArgType = CmdSetSdkRegDumpTraits::ObjectArgType;
  using RetType = CmdSetSdkRegDumpTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& state);

  void printOutput(const RetType& model, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
