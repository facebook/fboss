/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/route/CmdShowRouteSummary.h"

namespace facebook::fboss {

CmdShowRouteSummary::RetType CmdShowRouteSummary::queryClient(
    const HostInfo& hostInfo) {
  std::vector<facebook::fboss::UnicastRoute> entries;
  auto client =
      utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

  client->sync_getRouteTable(entries);
  return createModel(entries);
}

void CmdShowRouteSummary::printOutput(const RetType& model, std::ostream& out) {
  out << "Route Table Summary:\n\n";
  out << fmt::format(
      "{:-10} v4 routes\n"
      "{:-10} v6 routes (/64 or smaller)\n"
      "{:-10} v6 routes (bigger than /64)\n"
      "{:-10} v6 routes (total)\n"
      "{:-10} approximate hw entries used\n\n",
      folly::copy(model.numV4Routes().value()),
      folly::copy(model.numV6Small().value()),
      folly::copy(model.numV6Big().value()),
      folly::copy(model.numV6().value()),
      folly::copy(model.hwEntriesUsed().value()));
}

CmdShowRouteSummary::RetType CmdShowRouteSummary::createModel(
    const std::vector<facebook::fboss::UnicastRoute>& routeEntries) {
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

} // namespace facebook::fboss
