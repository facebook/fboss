// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/CmdShowInterfaceCountersFec.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/ber/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdShowInterfaceCountersFecBerTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowInterfaceCountersFec;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowInterfaceCountersFecBerModel;
};

class CmdShowInterfaceCountersFecBer
    : public CmdHandler<
          CmdShowInterfaceCountersFecBer,
          CmdShowInterfaceCountersFecBerTraits> {
 public:
  using ObjectArgType = CmdShowInterfaceCountersFecBerTraits::ObjectArgType;
  using RetType = CmdShowInterfaceCountersFecBerTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::PortList& queriedIfs,
      const utils::LinkDirection& direction);

  void printOutput(const RetType& model, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
