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
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

#include <folly/executors/IOThreadPoolExecutor.h>

namespace facebook::fboss {

struct CmdShowInterfaceTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = std::map<int32_t, facebook::fboss::PortInfoThrift>;
};

class CmdShowInterface
    : public CmdHandler<CmdShowInterface, CmdShowInterfaceTraits> {
 public:
  using RetType = CmdShowInterfaceTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedPorts) {
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    folly::IOThreadPoolExecutor executor(4);

    auto portInfos =
        client->semifuture_getAllPortInfo()
            .deferValue([&](auto portInfo) {
              if (queriedPorts.size() == 0) {
                return std::move(portInfo);
              }

              // TODO: move this to a utility since it
              // seems we'll need to get filtered ports a lot

              std::map<int32_t, facebook::fboss::PortInfoThrift> filteredPorts;
              for (auto const& [portId, portInfo] : portInfo) {
                for (auto const& queriedPort : queriedPorts) {
                  if (portInfo.get_name() == queriedPort) {
                    filteredPorts.insert(std::make_pair(portId, portInfo));
                  }
                }
              }
              return filteredPorts;
            })
            .via(executor.getEventBase());
    auto interfaceDetails =
        client->semifuture_getAllInterfaces().via(executor.getEventBase());
    auto counters =
        client->semifuture_getCounters().via(executor.getEventBase());
    auto getAggregatePortTable =
        client->semifuture_getAggregatePortTable().via(executor.getEventBase());

    portInfos.wait();
    interfaceDetails.wait();
    counters.wait();
    getAggregatePortTable.wait();

    return portInfos.value();
  }

  // TODO: implement output
  void printOutput(const RetType& ports) {
    for (const auto& portItr : ports) {
      std::cout << portItr.second.get_name() << std::endl;
    }
  }
};

} // namespace facebook::fboss
