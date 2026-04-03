// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/CmdShowInterfaceCountersFec.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/fec/uncorrectable/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdShowInterfaceCountersFecUncorrectableTraits
    : public ReadCommandTraits {
  using ParentCmd = CmdShowInterfaceCountersFec;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowInterfaceCountersFecUncorrectableModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowInterfaceCountersFecUncorrectable
    : public CmdHandler<
          CmdShowInterfaceCountersFecUncorrectable,
          CmdShowInterfaceCountersFecUncorrectableTraits> {
 public:
  using ObjectArgType =
      CmdShowInterfaceCountersFecUncorrectableTraits::ObjectArgType;
  using RetType = CmdShowInterfaceCountersFecUncorrectableTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs,
      const utils::LinkDirection& direction);

  void printOutput(const RetType& model, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
