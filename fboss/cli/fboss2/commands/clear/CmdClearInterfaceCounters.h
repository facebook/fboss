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
#include "fboss/cli/fboss2/commands/clear/interface/CmdClearInterface.h"

#include <algorithm>
#include <string>
#include <variant>

namespace facebook::fboss {

struct CmdClearInterfaceCountersTraits : public WriteCommandTraits {
  using ParentCmd = CmdClearInterface;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

class CmdClearInterfaceCounters : public CmdHandler<
                                      CmdClearInterfaceCounters,
                                      CmdClearInterfaceCountersTraits> {
 public:
  using RetType = CmdClearInterfaceCountersTraits::RetType;

  std::string queryClient(
      const HostInfo& hostInfo,
      std::vector<std::string> queriedIfs) {
    std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    std::vector<int32_t> portsToClear;
    /*
      if the user has provided specific interfaces to clear then find the
      corresponding port names, otherwise empty vector means clear all counters
    */
    if (queriedIfs.size()) {
      client->sync_getAllPortInfo(portEntries);
      for (auto queriedIf : queriedIfs) {
        auto it = std::find_if(
            portEntries.begin(),
            portEntries.end(),
            [&queriedIf](const auto& port) {
              return port.second.get_name() == queriedIf;
            });
        if (it != portEntries.end()) {
          portsToClear.push_back(it->first);
        } else {
          throw std::runtime_error(
              fmt::format(
                  "{} is not a valid interface name on this device",
                  queriedIf));
        }
      }
    }
    clearCounters(*client, portsToClear);

    return "Counters cleared.";
  }

  void printOutput(const RetType& logMsg) {
    std::cout << logMsg << std::endl;
  }

  void clearCounters(FbossCtrlAsyncClient& client, std::vector<int32_t> ports) {
    if (ports.size()) {
      client.sync_clearPortStats(ports);
    } else {
      client.sync_clearAllPortStats();
    }
  }
};
} // namespace facebook::fboss
