// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/CmdShowInterfacePrbs.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/stats/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/PrbsUtils.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace facebook::fboss {

struct CmdShowInterfacePrbsStatsTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowInterfacePrbs;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowPrbsStatsModel;
};

class CmdShowInterfacePrbsStats : public CmdHandler<
                                      CmdShowInterfacePrbsStats,
                                      CmdShowInterfacePrbsStatsTraits> {
 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs,
      const std::vector<std::string>& components);

  void printOutput(const RetType& model, std::ostream& out = std::cout);

  std::map<std::string, phy::PrbsStats> getComponentPrbsStats(
      const HostInfo& hostInfo,
      const phy::PortComponent& component);

  template <class Client>
  void queryPrbsStats(
      std::unique_ptr<Client> client,
      std::map<std::string, phy::PrbsStats>& prbsStats,
      const phy::PortComponent& component) {
    try {
      client->sync_getAllInterfacePrbsStats(prbsStats, component);
    } catch (const std::exception& e) {
      std::cerr << e.what();
    }
  }

 private:
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries_;
};

} // namespace facebook::fboss
