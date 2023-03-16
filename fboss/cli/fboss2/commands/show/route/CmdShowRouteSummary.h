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

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <cstdint>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/route/CmdShowRoute.h"
#include "fboss/cli/fboss2/commands/show/route/gen-cpp2/model_types.h"

namespace facebook::fboss {

struct CmdShowRouteSummaryTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ParentCmd = CmdShowRoute;
  using ObjectArgType = utils::NoneArgType;
  using RetType = cli::ShowRouteSummaryModel;
};

class CmdShowRouteSummary
    : public CmdHandler<CmdShowRouteSummary, CmdShowRouteSummaryTraits> {
 public:
  RetType queryClient(const HostInfo& hostInfo) {
    std::vector<facebook::fboss::UnicastRoute> entries;
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    client->sync_getRouteTable(entries);
    return createModel(entries);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    out << "Route Table Summary:\n\n";
    out << fmt::format(
        "{:-10} v4 routes\n"
        "{:-10} v6 routes (/64 or smaller)\n"
        "{:-10} v6 routes (bigger than /64)\n"
        "{:-10} v6 routes (total)\n"
        "{:-10} approximate hw entries used\n\n",
        model.get_numV4Routes(),
        model.get_numV6Small(),
        model.get_numV6Big(),
        model.get_numV6(),
        model.get_hwEntriesUsed());
  }

  RetType createModel(std::vector<facebook::fboss::UnicastRoute> routeEntries) {
    RetType model;
    int numV4Routes = 0, numV6Small = 0, numV6Big = 0;

    for (const auto& entry : routeEntries) {
      auto ip = *entry.dest()->ip()->addr();
      if (ip.size() == 4) {
        ++numV4Routes;
      } else if (ip.size() == 16) {
        // break ipv6 up into <64 and >64  as it affect ASIC mem usage
        if (*entry.dest()->prefixLength() <= 64) {
          ++numV6Small;
        } else {
          ++numV6Big;
        }
      }
      model.numV4Routes() = numV4Routes;
      model.numV6Small() = numV6Small;
      model.numV6Big() = numV6Big;
      model.numV6() = numV6Big + numV6Small;
      model.hwEntriesUsed() = numV4Routes + 2 * numV6Small + 4 * numV6Big;
    }
    return model;
  }
};

} // namespace facebook::fboss
