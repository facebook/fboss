/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/bgp/holdtimers/CmdShowBgpHoldTimers.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

namespace facebook::fboss {

CmdShowBgpHoldTimers::RetType CmdShowBgpHoldTimers::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& queriedPeers) {
  RetType holdTimers;

  auto client = utils::createClient<apache::thrift::Client<
      facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

  client->sync_getHoldTimers(holdTimers, queriedPeers);
  return holdTimers;
}

void CmdShowBgpHoldTimers::printOutput(const RetType& data, std::ostream& out) {
  if (data.empty()) {
    out << "No hold timer information available" << std::endl;
    return;
  }

  utils::Table table;
  table.setHeader({"Peer Address", "Hold Time Remaining (ms)"});

  for (const auto& info : data) {
    table.addRow({
        *info.peer_address(),
        folly::to<std::string>(*info.hold_time_remaining_ms()),
    });
  }
  out << table << std::endl;
}

template void
CmdHandler<CmdShowBgpHoldTimers, CmdShowBgpHoldTimersTraits>::run();

} // namespace facebook::fboss
