// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/CmdShowInterface.h"
#include "fboss/cli/fboss2/commands/show/interface/phy/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace facebook::fboss {

struct CmdShowInterfacePhyTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowInterface;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_PHY_CHIP_TYPE;
  using ObjectArgType = utils::PhyChipType;
  using RetType = cli::ShowInterfacePhyModel;
};

class CmdShowInterfacePhy
    : public CmdHandler<CmdShowInterfacePhy, CmdShowInterfacePhyTraits> {
 public:
  using ObjectArgType = CmdShowInterfacePhyTraits::ObjectArgType;
  using RetType = CmdShowInterfacePhyTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::PortList& queriedIfs,
      const utils::PhyChipType& phyChipType);
  void printOutput(const RetType& model, std::ostream& out = std::cout);
  void printPhyInfo(
      std::ostream& out,
      phy::PhyInfo& phyInfo,
      const std::string& ifName);
  void printSideStateAndStat(
      std::ostream& out,
      phy::PhySideState& sideState,
      phy::PhySideStats& sideStats,
      const std::string& prefix);
  void printPmdLaneRxInfo(
      std::ostream& out,
      phy::PhySideState& sideState,
      phy::PhySideStats& sideStats,
      const std::set<int>& pmdLanes,
      const std::string& prefix);
  void printPmdLaneTxInfo(
      std::ostream& out,
      phy::PhySideState& sideState,
      const std::set<int>& pmdLanes,
      const std::string& prefix);
  void printSerdesParametersInfo(
      std::ostream& out,
      phy::PmdState& pmdState,
      const std::string& prefix);
  Table::StyledCell makeColorCellForLiveFlag(const std::string& flag);
  RetType createModel(
      const HostInfo& hostInfo,
      const utils::PortList& queriedIfs,
      const utils::PhyChipType& phyChipType);
};

} // namespace facebook::fboss
