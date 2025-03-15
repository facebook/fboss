// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/example/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

/*
  Since we don't want to open source this command, it is place under a facebook
  directory. All files under these directories are not synced to github. As
  such, deal with this some parts of the code have 2 implementations, one in a
  normal directory and one in a facebook directory. Most notably CmdList.cpp and
  CmdHandler.cpp have 2 versions and depending on if a new command is
  opensourcable, the corresponding version needs to be modified.

  NOTE: if a command is opensourced, we also need to modify the cmake file at
  fboss/github/cmake/CliFboss2.cmake
*/
namespace facebook::fboss {

using utils::Table;

/*
 Define the traits of this command. This will include the inputs and output
 types
*/
struct CmdShowExampleTraits : public BaseCommandTraits {
  // The object type that the command accepts
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  // The actual c++ type of the input
  using ObjectArgType = std::vector<std::string>;
  // The return type of the command, must be thrift or otherwise serializable
  // (vector, map, string, pimitive)
  using RetType = cli::ShowExampleModel;
};

class CmdShowExample : public CmdHandler<CmdShowExample, CmdShowExampleTraits> {
 public:
  using ObjectArgType = CmdShowExampleTraits::ObjectArgType;
  using RetType = CmdShowExampleTraits::RetType;

  /*
   Query thrift clients and return normalized output
  */
  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& /* queriedPorts */) {
    // Create thrift clients to query agents
    auto wedgeAgent = utils::createClient<FbossCtrlAsyncClient>(hostInfo);
    auto qsfpService = utils::createClient<QsfpServiceAsyncClient>(hostInfo);

    // fetch the required data
    std::map<int32_t, PortInfoThrift> entries;
    wedgeAgent->sync_getAllPortInfo(entries);

    // do filtering and some minimal processing

    return createModel(entries);
  }

  /*
    Run data normalization to linked thrift object
  */
  RetType createModel(std::map<int32_t, PortInfoThrift> data) {
    // Go through the queried data and normalize
    RetType ret;
    for (const auto& [key, queriedItem] : data) {
      cli::ExampleData normalizedItem;
      normalizedItem.id() = key;
      normalizedItem.name() = queriedItem.get_name();
      ret.exampleData()->push_back(normalizedItem);
    }
    return ret;
  }

  /*
    Output to human readable format
  */
  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    // Recommended to use the Table library if suitable
    Table table;
    table.setHeader({"ID", "Name"});

    for (auto const& data : model.get_exampleData()) {
      table.addRow({
          folly::to<std::string>(data.get_id()),
          // Can conditionally add styles to cells
          data.get_name() != ""
              ? data.get_name()
              : Table::StyledCell("No Name!", Table::Style::WARN),
      });
    }

    out << table << std::endl;
  }
};

} // namespace facebook::fboss
