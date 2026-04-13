// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <cstdint>
#include <map>

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/clear/interface/prbs/CmdClearInterfacePrbs.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace facebook::fboss {

struct CmdClearInterfacePrbsStatsTraits : public WriteCommandTraits {
  using ParentCmd = CmdClearInterfacePrbs;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

class CmdClearInterfacePrbsStats : public CmdHandler<
                                       CmdClearInterfacePrbsStats,
                                       CmdClearInterfacePrbsStatsTraits> {
 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs,
      const std::vector<std::string>& components);

  void printOutput(const RetType& model, std::ostream& out = std::cout);

  RetType createModel(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs,
      const std::vector<phy::PortComponent>& components);

  void clearComponentPrbsStats(
      const HostInfo& hostInfo,
      const std::vector<std::string>& interfaces,
      const phy::PortComponent& component);

  template <class Client>
  void queryClearPrbsStats(
      std::unique_ptr<Client> client,
      const std::vector<std::string>& interfaces,
      const phy::PortComponent& component) {
    try {
      client->sync_bulkClearInterfacePrbsStats(interfaces, component);
    } catch (const std::exception& e) {
      std::cerr << e.what();
    }
  }

 private:
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries_;
};

} // namespace facebook::fboss
