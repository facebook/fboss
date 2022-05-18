// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/String.h>
#include "fboss/cli/fboss2/CmdHandler.h"

namespace facebook::fboss {

struct CmdSetInterfaceTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = std::string;
};

class CmdSetInterface
    : public CmdHandler<CmdSetInterface, CmdSetInterfaceTraits> {
 public:
  RetType queryClient(
      const HostInfo& /* hostInfo */,
      const std::vector<std::string>& /* queriedIfs */) {
    throw std::runtime_error(
        "Incomplete command, please use one the subcommands");
    return RetType();
  }

  void printOutput(const RetType& /* model */) {}
};

} // namespace facebook::fboss
