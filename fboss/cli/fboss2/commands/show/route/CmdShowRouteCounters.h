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

#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/route/CmdShowRoute.h"
#include "fboss/cli/fboss2/commands/show/route/gen-cpp2/model_types.h"

#include <map>
#include <string>
#include <string_view>

namespace facebook::fboss {

struct CmdShowRouteCountersTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ParentCmd = CmdShowRoute;
  using ObjectArgType = utils::NoneArgType;
  using RetType = cli::ShowRouteCountersModel;

  static std::string_view description();
};

class CmdShowRouteCounters
    : public CmdHandler<CmdShowRouteCounters, CmdShowRouteCountersTraits> {
 public:
  using RetType = CmdShowRouteCountersTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);
  RetType createModel(
      const std::map<std::string, HwSwitchCounter>& routeCounters) const;
  void printOutput(const RetType& model, std::ostream& out = std::cout);

  static RetType sampleModel();
};

} // namespace facebook::fboss
