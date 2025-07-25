// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "CmdShowExample.h"

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <map>
#include <string>
#include <vector>
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "folly/Conv.h"

namespace facebook::fboss {

using utils::Table;
using RetType = CmdShowExampleTraits::RetType;
using ObjectArgType = CmdShowExampleTraits::ObjectArgType;

RetType CmdShowExample::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& /* queriedPorts */) {
  // Create thrift clients to query agents
  auto wedgeAgent =
      utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);
  auto qsfpService =
      utils::createClient<facebook::fboss::QsfpServiceAsyncClient>(hostInfo);

  // fetch the required data
  std::map<int32_t, facebook::fboss::PortInfoThrift> entries;
  wedgeAgent->sync_getAllPortInfo(entries);

  // do filtering and some minimal processing

  return createModel(entries);
}

RetType CmdShowExample::createModel(
    std::map<int32_t, facebook::fboss::PortInfoThrift>& data) {
  // Go through the queried data and normalize
  RetType ret;
  for (const auto& [key, queriedItem] : data) {
    cli::ExampleData normalizedItem;
    normalizedItem.id() = key;
    normalizedItem.name() = queriedItem.name().value();
    ret.exampleData()->push_back(normalizedItem);
  }
  return ret;
}

void CmdShowExample::printOutput(const RetType& model, std::ostream& out) {
  // Recommended to use the Table library if suitable
  Table table;
  table.setHeader({"ID", "Name"});

  for (auto const& data : model.exampleData().value()) {
    table.addRow({
        folly::to<std::string>(folly::copy(data.id().value())),
        // Can conditionally add styles to cells
        data.name().value() != ""
            ? data.name().value()
            : Table::StyledCell("No Name!", Table::Style::WARN),
    });
  }

  out << table << std::endl;
}

} // namespace facebook::fboss
