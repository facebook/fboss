// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/set/interface/CmdSetInterface.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdSetInterfacePrbsTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_PRBS_COMPONENT;
  using ParentCmd = CmdSetInterface;
  using ObjectArgType = utils::PrbsComponent;
  using RetType = std::string;
};

class CmdSetInterfacePrbs
    : public CmdHandler<CmdSetInterfacePrbs, CmdSetInterfacePrbsTraits> {
 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::PortList& queriedIfs,
      const ObjectArgType& components);

  void printOutput(const RetType& model);
};

} // namespace facebook::fboss
