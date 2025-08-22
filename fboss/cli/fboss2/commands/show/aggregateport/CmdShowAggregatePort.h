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
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/aggregateport/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/commands/show/aggregateport/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdShowAggregatePortTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = cli::ShowAggregatePortModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowAggregatePort
    : public CmdHandler<CmdShowAggregatePort, CmdShowAggregatePortTraits> {
 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedPorts);

  void printOutput(const RetType& model, std::ostream& out = std::cout);

  RetType createModel(
      const std::vector<facebook::fboss::AggregatePortThrift>&
          aggregatePortEntries,
      std::map<int32_t, facebook::fboss::PortInfoThrift> portInfo,
      const ObjectArgType& queriedPorts);
};

} // namespace facebook::fboss
