// Copyright 2004-present Facebook. All Rights Reserved.

#include "CmdShowFabricMonitoringDetails.h"

#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

using utils::Table;
using ObjectArgType = CmdShowFabricMonitoringDetailsTraits::ObjectArgType;
using RetType = CmdShowFabricMonitoringDetailsTraits::RetType;

RetType CmdShowFabricMonitoringDetails::queryClient(const HostInfo& hostInfo) {
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

  std::vector<FabricMonitoringDetail> agentDetails;
  client->sync_getFabricMonitoringDetails(agentDetails);

  return createModel(agentDetails);
}

RetType CmdShowFabricMonitoringDetails::createModel(
    const std::vector<FabricMonitoringDetail>& agentDetails) {
  RetType model;

  for (const auto& agentDetail : agentDetails) {
    cli::FabricMonitoringDetail detail;

    detail.portName() = *agentDetail.portName();
    detail.portId() = *agentDetail.portId();
    detail.neighborSwitch() = *agentDetail.neighborSwitch();
    detail.neighborPortName() = *agentDetail.neighborPortName();
    detail.virtualDevice() = *agentDetail.virtualDevice();
    detail.linkSwitchId() = *agentDetail.linkSwitchId();
    detail.linkSystemPort() = *agentDetail.linkSystemPort();

    model.details()->push_back(detail);
  }

  std::sort(
      model.details()->begin(),
      model.details()->end(),
      [](const cli::FabricMonitoringDetail& counter1,
         const cli::FabricMonitoringDetail& counter2) {
        return utils::comparePortName(
            *counter1.portName(), *counter2.portName());
      });

  return model;
}

void CmdShowFabricMonitoringDetails::printOutput(
    const RetType& model,
    std::ostream& out) {
  Table table;
  table.setHeader({
      "Port Name",
      "Port ID",
      "Neighbor Switch",
      "Neighbor Port",
      "Virtual Device",
      "Link Switch ID",
      "System Port",
  });

  for (const auto& detail : *model.details()) {
    auto formatInt64 = [](int64_t value) -> std::string {
      return (value == -1) ? "--" : std::to_string(value);
    };

    auto formatString = [](const std::string& value) -> std::string {
      return value.empty() ? "--" : value;
    };

    table.addRow({
        formatString(*detail.portName()),
        formatInt64(*detail.portId()),
        formatString(*detail.neighborSwitch()),
        formatString(*detail.neighborPortName()),
        formatInt64(*detail.virtualDevice()),
        formatInt64(*detail.linkSwitchId()),
        formatString(*detail.linkSystemPort()),
    });
  }

  out << table << std::endl;
}

} // namespace facebook::fboss
