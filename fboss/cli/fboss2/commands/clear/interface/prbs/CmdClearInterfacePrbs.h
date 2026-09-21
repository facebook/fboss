// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/clear/interface/CmdClearInterface.h"

namespace facebook::fboss {

struct CmdClearInterfacePrbsTraits : public WriteCommandTraits {
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
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs,
      const ObjectArgType& components);

  void printOutput(const RetType& model);
};

} // namespace facebook::fboss
