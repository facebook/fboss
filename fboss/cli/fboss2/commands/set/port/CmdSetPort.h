// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/String.h>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdSetPortTraits : public BaseCommandTraits {
  using ObjectArgType = utils::PortList;
  using RetType = std::string;
};

class CmdSetPort : public CmdHandler<CmdSetPort, CmdSetPortTraits> {
 public:
  RetType queryClient(
      const HostInfo& /* hostInfo */,
      const ObjectArgType& /* queriedPortIds */) {
    throw std::runtime_error(
        "Incomplete command, please use one the subcommands");
    return RetType();
  }

  void printOutput(const RetType& /* model */) {}
};

} // namespace facebook::fboss
