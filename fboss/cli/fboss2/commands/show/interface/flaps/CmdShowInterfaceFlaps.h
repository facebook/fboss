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

#include <string_view>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/interface/CmdShowInterface.h"
#include "fboss/cli/fboss2/commands/show/interface/flaps/gen-cpp2/model_types.h"

namespace facebook::fboss {

struct CmdShowInterfaceFlapsTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowInterface;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::InterfaceFlapsModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;

  // Human-authored guide prose for the CLI reference wiki. Superset of the
  // one-line help string registered in the command tree.
  static std::string_view description();
};

CmdShowInterfaceFlapsTraits::RetType createModel(
    std::vector<std::string>& ifNames,
    std::map<std::string, std::int64_t> wedgeCounters,
    const std::vector<std::string>& queriedIfs);

class CmdShowInterfaceFlaps
    : public CmdHandler<CmdShowInterfaceFlaps, CmdShowInterfaceFlapsTraits> {
 public:
  using ObjectArgType = CmdShowInterfaceFlapsTraits::ObjectArgType;
  using RetType = CmdShowInterfaceFlapsTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIfs);

  void printOutput(const RetType& model, std::ostream& out = std::cout);

  // Canned, synthetic model (no real switch data) used to render a
  // deterministic example for the CLI reference wiki. No live switch.
  static RetType sampleModel();
};

} // namespace facebook::fboss
