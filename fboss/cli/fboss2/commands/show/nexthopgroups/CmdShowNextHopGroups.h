// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string_view>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/nexthopgroups/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdShowNextHopGroupsTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = utils::NoneArgType;
  using RetType = cli::ShowNextHopGroupsModel;

  // Human-authored guide prose for the CLI reference wiki. Superset of the
  // one-line help string registered in the command tree.
  static std::string_view description();
};

struct CmdShowNamedNextHopGroupsTraits : public CmdShowNextHopGroupsTraits {};

class CmdShowNextHopGroups
    : public CmdHandler<CmdShowNextHopGroups, CmdShowNextHopGroupsTraits> {
 public:
  using RetType = CmdShowNextHopGroupsTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);
  RetType createModel(
      const std::vector<NextHopGroup>& nextHopGroups,
      bool namedOnly = false);
  void printOutput(const RetType& model, std::ostream& out = std::cout);

  // Canned, synthetic model (no real switch data) used to render a
  // deterministic example for the CLI reference wiki. No live switch.
  static RetType sampleModel();
};

class CmdShowNamedNextHopGroups : public CmdHandler<
                                      CmdShowNamedNextHopGroups,
                                      CmdShowNamedNextHopGroupsTraits> {
 public:
  using RetType = CmdShowNamedNextHopGroupsTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);
  RetType createModel(const std::vector<NextHopGroup>& nextHopGroups);
  void printOutput(const RetType& model, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
