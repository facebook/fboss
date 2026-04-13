// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/CmdShowInterfaceCounters.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdShowInterfaceCountersFecTraits : public ReadCommandTraits {
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
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs,
      const ObjectArgType& direction);

  void printOutput(const RetType& model);
};

} // namespace facebook::fboss
