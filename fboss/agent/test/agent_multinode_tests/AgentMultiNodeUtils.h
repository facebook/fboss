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
#include "fboss/agent/types.h"

#include <folly/MacAddress.h>

namespace facebook::fboss::utility {
struct Neighbor {
  int32_t portID = 0;
  int32_t intfID = 0;
  folly::IPAddress ip = folly::IPAddress("::");
  folly::MacAddress mac = folly::MacAddress("00:00:00:00:00:00");

  std::string str() const {
    return folly::to<std::string>(
        "portID: ",
        portID,
        ", intfID: ",
        intfID,
        ", ip: ",
        ip.str(),
        ", mac: ",
        mac.toString());
  }
};

// Invoke the provided func on every element of a given container
// except on exclusions
template <typename Container, typename Callable, typename... Args>
void forEachExcluding(
    const Container& input,
    const Container& exclusions,
    Callable&& func,
    Args&&... args) {
  auto argsTuple = std::make_tuple(std::forward<Args>(args)...);
  for (const auto& elem : input) {
    if (exclusions.find(elem) == exclusions.end()) {
      std::apply(
          [&](auto&&... unpackedArgs) {
            func(elem, std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
          },
          argsTuple);
    }
  }
}

// Invoke the provided func on every element of a given container
// Return a map of func call result keyed by the element
template <typename Container, typename Func, typename... Args>
auto forEachWithRetVal(
    const Container& container,
    Func&& func,
    Args&&... args) {
  using KeyType = typename Container::value_type;
  using MappedType = std::invoke_result_t<Func, KeyType, Args...>;
  std::map<KeyType, MappedType> result;
  auto argsTuple = std::make_tuple(std::forward<Args>(args)...);
  for (const auto& elem : container) {
    result[elem] = std::apply(
        [&](auto&&... unpackedArgs) {
          return func(
              elem, std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
        },
        argsTuple);
  }
  return result;
}

// Invoke the provided func on every element of given set.
// except on exclusions.
// Return true if and only if every func returns true.
// false otherwise. Stops on first failure.
template <typename Container, typename Callable, typename... Args>
bool checkForEachExcluding(
    const Container& inputSet,
    const std::set<std::string>& exclusions,
    Callable&& func,
    Args&&... args) {
  // Store arguments in a tuple for safe repeated use
  auto argsTuple = std::make_tuple(std::forward<Args>(args)...);
  for (const auto& elem : inputSet) {
    if (exclusions.find(elem) == exclusions.end()) {
      bool result = std::apply(
          [&](auto&&... unpackedArgs) {
            return func(
                elem, std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
          },
          argsTuple);
      if (!result) {
        return false;
      }
    }
  }
  return true;
}

// Use provided function to restart service on all switches.
// Leverage getAliveSinceEpochFunc to determine service restart.
// Leverage verifyServiceRunState to verify service run state.
// Return true on success, false otherwise.
template <typename VerifyServiceRunState, typename... Args>
bool restartServiceForSwitches(
    const std::set<std::string>& switches,
    const std::function<void(const std::string&)>& restartServiceFunc,
    const std::function<int64_t(const std::string&)>& getAliveSinceEpochFunc,
    VerifyServiceRunState&& verifyServiceRunState,
    Args&&... args) {
  // Save baseline aliveSinceEpoch for all switches
  const auto& baselineSwitchToAliveSinceEpoch =
      forEachWithRetVal(switches, getAliveSinceEpochFunc);

  // Restart services on all switches
  forEachExcluding(switches, {} /* exclude none */, restartServiceFunc);

  auto allRestarted = [switches,
                       getAliveSinceEpochFunc,
                       baselineSwitchToAliveSinceEpoch] {
    const auto& currentSwitchToAliveSinceEpoch =
        forEachWithRetVal(switches, getAliveSinceEpochFunc);

    return std::all_of(
        currentSwitchToAliveSinceEpoch.begin(),
        currentSwitchToAliveSinceEpoch.end(),
        [&](const auto& pair) {
          const auto& switchName = pair.first;
          const auto& aliveSinceEpoch = pair.second;
          auto baselineIt = baselineSwitchToAliveSinceEpoch.find(switchName);
          CHECK(baselineIt != baselineSwitchToAliveSinceEpoch.end());
          // Clock is not guaranteed to be monitonic, thus check for inequality
          // rather than aliveSinceEpoch greater than the baseline.
          return baselineIt->second != aliveSinceEpoch;
        });
  };

  // Verify service restarted on all switches
  if (!checkWithRetryErrorReturn(
          allRestarted,
          30 /* num retries */,
          std::chrono::milliseconds(5000) /* sleep between retries */,
          true /* retry on exception */)) {
    XLOG(ERR) << "Failed to restart service for switches: ";
    return false;
  }

  // Verify service restarted on all switches
  return checkForEachExcluding(
      switches,
      {},
      std::forward<VerifyServiceRunState>(verifyServiceRunState),
      std::forward<Args>(args)...);
}
std::vector<NdpEntryThrift> getNdpEntriesOfType(
    const std::string& rdsw,
    const std::set<std::string>& types);

std::map<std::string, PortInfoThrift> getUpEthernetPortNameToPortInfo(
    const std::string& switchName);

int64_t getPortOutBytes(
    const std::string& switchName,
    const std::string& portName);

bool verifySwSwitchRunState(
    const std::string& switchName,
    const SwitchRunState& expectedSwitchRunState);

bool verifyQsfpServiceRunState(
    const std::string& switchName,
    const QsfpServiceRunState& expectedQsfpRunState);

bool verifyFsdbIsUp(const std::string& switchName);

bool verifyNeighborsPresent(
    const std::set<std::string>& allSwitches,
    const std::string& switchToVerify,
    const std::vector<Neighbor>& neighbors);

bool verifyNeighborsAbsent(
    const std::set<std::string>& allSwitches,
    const std::vector<Neighbor>& neighbors,
    const std::set<std::string>& switchesToExclude);

// Return true only if every neighbor in neighbors is present
// as local entry on the given switch.
bool verifyNeighborsLocallyPresent(
    const std::string& switchName,
    const std::vector<Neighbor>& neighbors);

// Return true only if every neighbor in neighbors is locally
// present but absent from every remote.
bool verifyNeighborsLocallyPresentRemoteAbsent(
    const std::set<std::string>& allSwitches,
    const std::vector<Neighbor>& neighbors,
    const std::string& switchName);

bool verifyRoutePresent(
    const std::string& switchName,
    const folly::IPAddress& destPrefix,
    const int16_t prefixLength);

bool verifyLineRate(const std::string& switchName, int32_t portID);

bool verifyPortOutBytesIncrementByMinValue(
    const std::string& switchName,
    const std::map<std::string, int64_t>& beforePortToOutBytes,
    const int64_t& minIncrement);

} // namespace facebook::fboss::utility
