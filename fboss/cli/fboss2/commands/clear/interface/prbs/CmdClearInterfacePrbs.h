// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <folly/String.h>
#include <folly/gen/Base.h>
#include <cstdint>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/clear/interface/CmdClearInterface.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/lib/phy/gen-cpp2/prbs_types.h"

namespace facebook::fboss {

struct CmdClearInterfacePrbsTraits : public BaseCommandTraits {
  using ParentCmd = CmdClearInterface;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_PRBS_COMPONENT;
  using ObjectArgType = std::vector<std::string>;
  using RetType = std::string;
};

class CmdClearInterfacePrbs
    : public CmdHandler<CmdClearInterfacePrbs, CmdClearInterfacePrbsTraits> {
 public:
  RetType queryClient(
      const HostInfo& /* hostInfo */,
      const std::vector<std::string>& /* queriedIfs */,
      const ObjectArgType& /* components */) {
    throw std::runtime_error(
        "Incomplete command, please use one the subcommands");
    return RetType();
  }

  void printOutput(const RetType& /* model */) {}
};

} // namespace facebook::fboss
