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

#include <folly/IPAddress.h>
#include <algorithm>

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/clear/CmdClearUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdClearArpTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_CIDR_NETWORK;
  using ObjectArgType = utils::CIDRNetwork;
  using RetType = std::string;
};

class CmdClearArp : public CmdHandler<CmdClearArp, CmdClearArpTraits> {
 public:
  using RetType = CmdClearArpTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedNetworks) {
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);
    std::vector<facebook::fboss::ArpEntryThrift> arpEntries;
    client->sync_getArpTable(arpEntries);
    return utils::flushNeighborEntries(client, arpEntries, queriedNetworks);
  }

  void printOutput(const RetType& logMsg) {
    std::cout << logMsg << std::endl;
  }
};

} // namespace facebook::fboss
