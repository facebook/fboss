// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <folly/String.h>
#include <folly/gen/Base.h>
#include <cstdint>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/CmdShowInterface.h"
#include "fboss/cli/fboss2/commands/show/interface/phy/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {

struct CmdShowInterfacePhyTraits : public BaseCommandTraits {
  using ParentCmd = CmdShowInterface;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_PHY_CHIP_TYPE;
  using ObjectArgType = utils::PhyChipType;
  using RetType = cli::ShowInterfacePhyModel;
};

class CmdShowInterfacePhy
    : public CmdHandler<CmdShowInterfacePhy, CmdShowInterfacePhyTraits> {
 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::PortList& queriedIfs,
      const utils::PhyChipType& phyChipType) {
    return createModel(hostInfo, queriedIfs, phyChipType);
  }

  void printOutput(const RetType& /* model */, std::ostream& out = std::cout) {
    out << std::endl;
  }

  RetType createModel(
      const HostInfo& hostInfo,
      const utils::PortList& queriedIfs,
      const utils::PhyChipType& phyChipType) {
    RetType model;
    // Process IPHY first if it is requested. We want to display phy data
    // starting from IPHY then XPHY
    if (phyChipType.iphyIncluded) {
      auto agentClient =
          utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);
      std::map<std::string, phy::PhyInfo> phyInfo;
      agentClient->sync_getInterfacePhyInfo(phyInfo, queriedIfs.data());
      for (auto& interfacePhyInfo : phyInfo) {
        model.phyInfo_ref()[interfacePhyInfo.first].insert(
            {phy::DataPlanePhyChipType::IPHY, interfacePhyInfo.second});
      }
    }
    if (phyChipType.xphyIncluded) {
      auto qsfpClient = utils::createClient<QsfpServiceAsyncClient>(hostInfo);
      std::map<std::string, phy::PhyInfo> phyInfo;
      qsfpClient->sync_getInterfacePhyInfo(phyInfo, queriedIfs.data());
      for (auto& interfacePhyInfo : phyInfo) {
        model.phyInfo_ref()[interfacePhyInfo.first].insert(
            {phy::DataPlanePhyChipType::XPHY, interfacePhyInfo.second});
      }
    }
    return model;
  }
};

} // namespace facebook::fboss
