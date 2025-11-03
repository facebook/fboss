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

#include "fboss/agent/if/gen-cpp2/FbossHwCtrl.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"

#include <folly/MacAddress.h>

namespace facebook::fboss::utility {
struct Neighbor {
  int32_t portID = 0;
  int32_t intfID = 0;
  folly::IPAddress ip = folly::IPAddress("::");
  folly::MacAddress mac = folly::MacAddress("00:00:00:00:00:00");
};

// Invoke the provided func on every element of a given container
template <typename Container, typename Callable, typename... Args>
void forEach(const Container& input, Callable&& func, Args&&... args) {
  auto argsTuple = std::make_tuple(std::forward<Args>(args)...);
  for (const auto& elem : input) {
    std::apply(
        [&](auto&&... unpackedArgs) {
          func(elem, std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
        },
        argsTuple);
  }
}

std::vector<NdpEntryThrift> getNdpEntriesOfType(
    const std::string& rdsw,
    const std::set<std::string>& types);

std::map<std::string, PortInfoThrift> getUpEthernetPortNameToPortInfo(
    const std::string& switchName);

bool verifySwSwitchRunState(
    const std::string& switchName,
    const SwitchRunState& expectedSwitchRunState);
bool verifyQsfpServiceRunState(
    const std::string& switchName,
    const QsfpServiceRunState& expectedQsfpRunState);
bool verifyFsdbIsUp(const std::string& switchName);

} // namespace facebook::fboss::utility
