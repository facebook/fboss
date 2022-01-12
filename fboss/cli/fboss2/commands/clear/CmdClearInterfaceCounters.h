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

#include <algorithm>
#include <string>

namespace facebook::fboss {

struct CmdClearInterfaceCountersTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = int32_t;
};

class CmdClearInterfaceCounters : public CmdHandler<
                                      CmdClearInterfaceCounters,
                                      CmdClearInterfaceCountersTraits> {
 public:
  using RetType = CmdClearInterfaceCountersTraits::RetType;
  // I tried bool as the return type here but bool does not have a "write"
  // method so it wouldn't compile
  int32_t queryClient(
      const HostInfo& hostInfo,
      std::vector<std::string> queriedIfs) {
    std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    std::vector<int32_t> portsToClear;
    client->sync_getAllPortInfo(portEntries);

    /*
    Populate list of ports to clear.  The function in agent thrift does not
    accept wild cards.  This loop looks to see if the user has provided specific
    interfaces to clear.  If so only get the portId of those specific
    interfaces. Otherwise  populate all portIds and clear them.
    */
    for (auto port : portEntries) {
      int32_t numberOfQueries = queriedIfs.size();
      if (numberOfQueries == 0 ||
          std::find(
              queriedIfs.begin(), queriedIfs.end(), port.second.get_name()) !=
              queriedIfs.end()) {
        portsToClear.push_back(port.first);
      }
    }
    try {
      client->sync_clearPortStats(portsToClear);
      return 1;
    } catch (...) {
      return 0;
    }
  }

  void printOutput(int32_t result) {
    if (result == 1) {
      std::cout << "Counters cleared." << std::endl;
    } else {
      std::cout << "Something went wrong.  Counters not cleared." << std::endl;
    }
  }
};
} // namespace facebook::fboss
