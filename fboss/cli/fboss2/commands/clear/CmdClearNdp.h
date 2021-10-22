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

#include <folly/IPAddress.h>

namespace facebook::fboss {

struct CmdClearNdpTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

class CmdClearNdp : public CmdHandler<CmdClearNdp, CmdClearNdpTraits> {
 public:
  using RetType = CmdClearNdpTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo) {
    RetType retVal;
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    std::vector<facebook::fboss::NdpEntryThrift> ndpEntries;
    client->sync_getNdpTable(ndpEntries);

    for (auto const& ndpEntry : ndpEntries) {
      auto ip = folly::IPAddress::fromBinary(
          folly::ByteRange(folly::StringPiece(ndpEntry.get_ip().get_addr())));
      std::cout << "Deleting Ndp entry ip: " << ip.str()
                << " vlanID: " << ndpEntry.get_vlanID() << std::endl;
      client->sync_flushNeighborEntry(ndpEntry.get_ip(), ndpEntry.get_vlanID());
    }

    retVal = folly::to<std::string>("Flushed ", ndpEntries.size(), " entries");

    return retVal;
  }

  void printOutput(const RetType& logMsg) {
    std::cout << logMsg << std::endl;
  }
};

} // namespace facebook::fboss
