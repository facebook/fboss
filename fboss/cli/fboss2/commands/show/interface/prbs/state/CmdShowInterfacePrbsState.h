// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/CmdShowInterfacePrbs.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/state/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/PrbsUtils.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/lib/phy/gen-cpp2/prbs_types.h"

namespace facebook::fboss {

struct CmdShowInterfacePrbsStateTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowInterfacePrbs;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowPrbsStateModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowInterfacePrbsState : public CmdHandler<
                                      CmdShowInterfacePrbsState,
                                      CmdShowInterfacePrbsStateTraits> {
 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs,
      const std::vector<std::string>& components);

  void printOutput(const RetType& model, std::ostream& out = std::cout);

  std::map<std::string, prbs::InterfacePrbsState> getComponentPrbsStates(
      const HostInfo& hostInfo,
      const phy::PortComponent& component);

  template <class Client>
  void queryPrbsStates(
      std::unique_ptr<Client> client,
      std::map<std::string, prbs::InterfacePrbsState>& prbsStates,
      const phy::PortComponent& component) {
    try {
      client->sync_getAllInterfacePrbsStates(prbsStates, component);
    } catch (const std::exception& e) {
      std::cerr << e.what();
    }
  }
};

} // namespace facebook::fboss
