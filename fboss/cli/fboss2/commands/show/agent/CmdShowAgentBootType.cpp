// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "CmdShowAgentBootType.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

using RetType = CmdShowAgentBootTypeTraits::RetType;
using utils::Table;

RetType CmdShowAgentBootType::queryClient(const HostInfo& hostInfo) {
  auto client = utils::createAgentClient(hostInfo);

  MultiSwitchRunState runState;
  client->sync_getMultiSwitchRunState(runState);
  const auto isMultiSwitchEnabled = *runState.multiSwitchEnabled();

  std::vector<AgentBootType> bootTypes;
  bootTypes.push_back(
      AgentBootType{
          .agentName = isMultiSwitchEnabled ? "sw_agent" : "wedge_agent",
          .switchIndex = std::nullopt,
          .bootType = client->sync_getBootType(),
      });

  if (isMultiSwitchEnabled) {
    const auto numHwSwitches =
        static_cast<int32_t>(runState.hwIndexToRunState()->size());
    for (int32_t switchIndex = 0; switchIndex < numHwSwitches; ++switchIndex) {
      try {
        auto hwClient =
            utils::createClient<apache::thrift::Client<FbossHwCtrl>>(
                hostInfo, switchIndex);
        bootTypes.push_back(
            AgentBootType{
                .agentName = "hw_agent",
                .switchIndex = switchIndex,
                .bootType = hwClient->sync_getBootType(),
            });
      } catch (const std::exception&) {
        // Skip hw agents that cannot be reached.
      }
    }
  }

  return createModel(bootTypes);
}

RetType CmdShowAgentBootType::createModel(
    const std::vector<AgentBootType>& bootTypes) const {
  RetType model;
  for (const auto& bootType : bootTypes) {
    cli::AgentBootTypeEntry entry;
    entry.agentName() = bootType.agentName;
    if (bootType.switchIndex.has_value()) {
      entry.switchIndex() = bootType.switchIndex.value();
    }
    entry.bootType() = bootTypeToString(bootType.bootType);
    model.bootTypeEntries()->push_back(entry);
  }
  return model;
}

void CmdShowAgentBootType::printOutput(
    const RetType& model,
    std::ostream& out) {
  Table table;
  table.setHeader({"Agent", "SwitchIndex", "BootType"});
  for (const auto& entry : *model.bootTypeEntries()) {
    table.addRow({
        entry.agentName().value(),
        entry.switchIndex().has_value()
            ? std::to_string(entry.switchIndex().value())
            : "N/A",
        entry.bootType().value(),
    });
  }
  out << table << std::endl;
}

std::string CmdShowAgentBootType::bootTypeToString(BootType bootType) {
  return apache::thrift::util::enumNameSafe(bootType);
}

// Explicit template instantiation
template void
CmdHandler<CmdShowAgentBootType, CmdShowAgentBootTypeTraits>::run();

} // namespace facebook::fboss
