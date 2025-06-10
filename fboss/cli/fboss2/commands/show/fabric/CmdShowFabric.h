/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <map>
#include <string>
#include "fboss/agent/if/gen-cpp2/hw_ctrl_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/fabric/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

struct CmdShowFabricTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowFabricModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowFabric : public CmdHandler<CmdShowFabric, CmdShowFabricTraits> {
 public:
  using RetType = CmdShowFabricTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);
  inline void udpateNametoIdString(std::string& name, int64_t value);
  void printOutput(const RetType& model, std::ostream& out = std::cout);
  utils::Table::Style get_NeighborStyle(
      const std::string& actualId,
      const std::string& expectedId);
  RetType createModel(std::map<std::string, FabricEndpoint> fabricEntries);
};

} // namespace facebook::fboss
