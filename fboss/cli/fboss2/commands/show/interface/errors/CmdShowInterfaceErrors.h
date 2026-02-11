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

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/CmdShowInterface.h"
#include "fboss/cli/fboss2/commands/show/interface/errors/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

using utils::Table;

struct CmdShowInterfaceErrorsTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowInterface;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::InterfaceErrorsModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowInterfaceErrors
    : public CmdHandler<CmdShowInterfaceErrors, CmdShowInterfaceErrorsTraits> {
 public:
  using ObjectArgType = CmdShowInterfaceErrorsTraits::ObjectArgType;
  using RetType = CmdShowInterfaceErrorsTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs);
  RetType createModel(
      const std::map<int32_t, facebook::fboss::PortInfoThrift>& portCounters,
      const std::vector<std::string>& queriedIfs);
  Table::StyledCell stylizeCounterValue(int64_t counterValue);
  void printOutput(const RetType& model, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
