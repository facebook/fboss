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

#include <string>
#include <string_view>
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/if/gen-cpp2/hw_ctrl_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/fabric/topology/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

using utils::Table;

struct CmdShowFabricTopologyTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowFabricTopologyModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;

  // Human-authored guide prose for the CLI reference wiki. Superset of the
  // one-line help string registered in the command tree.
  static std::string_view description();
};

class CmdShowFabricTopology
    : public CmdHandler<CmdShowFabricTopology, CmdShowFabricTopologyTraits> {
 public:
  using RetType = CmdShowFabricTopologyTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);

  RetType createModel(
      const std::map<int64_t, std::map<int64_t, std::vector<RemoteEndpoint>>>&
          entries);

  void printOutput(const RetType& model, std::ostream& out = std::cout);

  Table::Style getSymmetryStyle(bool isSymmetric) const;

  // Canned, synthetic model (no real switch data) used to render a
  // deterministic example for the CLI reference wiki. No live switch.
  static RetType sampleModel();
};

} // namespace facebook::fboss
