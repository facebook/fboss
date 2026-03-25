// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/fabric/CmdShowFabric.h"
#include "fboss/cli/fboss2/commands/show/fabric/monitoring/gen-cpp2/model_types.h"

namespace facebook::fboss {

struct CmdShowFabricMonitoringDetailsTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowFabric;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::FabricMonitoringDetailsModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowFabricMonitoringDetails
    : public CmdHandler<
          CmdShowFabricMonitoringDetails,
          CmdShowFabricMonitoringDetailsTraits> {
 public:
  using ObjectArgType = CmdShowFabricMonitoringDetailsTraits::ObjectArgType;
  using RetType = CmdShowFabricMonitoringDetailsTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);

  void printOutput(const RetType& model, std::ostream& out = std::cout);

  RetType createModel(const std::vector<FabricMonitoringDetail>& agentDetails);
};

} // namespace facebook::fboss
