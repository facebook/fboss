// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/set/interface/prbs/CmdSetInterfacePrbs.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace facebook::fboss {

struct CmdSetInterfacePrbsStateTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_PRBS_STATE;
  using ParentCmd = CmdSetInterfacePrbs;
  using ObjectArgType = utils::PrbsState;
  using RetType = std::string;
};

class CmdSetInterfacePrbsState : public CmdHandler<
                                     CmdSetInterfacePrbsState,
                                     CmdSetInterfacePrbsStateTraits> {
 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::PortList& queriedIfs,
      const utils::PrbsComponent& components,
      const ObjectArgType& state);

  RetType createModel(
      const HostInfo& hostInfo,
      const utils::PortList& queriedIfs,
      const utils::PrbsComponent& components,
      const ObjectArgType& state);

  void setPrbsState(
      const HostInfo& hostInfo,
      const std::string& interfaceName,
      const phy::PortComponent& component,
      const ObjectArgType& state);

  void printOutput(const RetType& model, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
