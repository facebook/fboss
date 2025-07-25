// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <map>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/example/gen-cpp2/model_types.h"

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

/*
 Define the traits of this command. This will include the inputs and output
 types
*/
struct CmdShowExampleTraits : public ReadCommandTraits {
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
      const ObjectArgType& queriedPorts);

  /*
    Run data normalization to linked thrift object
  */
  RetType createModel(std::map<int32_t, facebook::fboss::PortInfoThrift>& data);

  /*
    Output to human readable format
  */
  void printOutput(const RetType& model, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
