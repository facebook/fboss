// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <fboss/cli/fboss2/utils/CmdUtils.h>
#include <folly/String.h>
#include <folly/gen/Base.h>
#include <cstdint>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/CmdShowInterfaceCounters.h"

namespace facebook::fboss {

struct CmdShowInterfaceCountersFecTraits : public BaseCommandTraits {
  using ParentCmd = CmdShowInterfaceCounters;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_LINK_DIRECTION;
  using ObjectArgType = utils::LinkDirection;
  using RetType = std::string;
};

class CmdShowInterfaceCountersFec : public CmdHandler<
                                        CmdShowInterfaceCountersFec,
                                        CmdShowInterfaceCountersFecTraits> {
 public:
  using ObjectArgType = CmdShowInterfaceCountersFecTraits::ObjectArgType;
  using RetType = CmdShowInterfaceCountersFecTraits::RetType;

  RetType queryClient(
      const HostInfo& /* hostInfo */,
      const std::vector<std::string>& /* queriedIfs */,
      const ObjectArgType& /* system|line */) {
    throw std::runtime_error(
        "Incomplete command, please use one the subcommands");
    return RetType();
  }

  void printOutput(const RetType& /* model */) {}
};

} // namespace facebook::fboss
