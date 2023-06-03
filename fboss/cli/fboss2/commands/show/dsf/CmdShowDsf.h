// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/String.h>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdShowDsfTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

class CmdShowDsf : public CmdHandler<CmdShowDsf, CmdShowDsfTraits> {
 public:
  RetType queryClient(const HostInfo& /* hostInfo */) {
    throw std::runtime_error(
        "Incomplete command, please use one the subcommands");
    return RetType();
  }

  void printOutput(const RetType& /* model */) {}
};

} // namespace facebook::fboss
