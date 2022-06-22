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

#include <CLI/CLI.hpp>
#include <folly/IPAddress.h>
#include <string>
#include <variant>
#include "fboss/agent/if/gen-cpp2/FbossCtrlAsyncClient.h"

namespace facebook::fboss::utils {

enum class ObjectArgTypeId : uint8_t {
  OBJECT_ARG_TYPE_ID_NONE = 0,
  OBJECT_ARG_TYPE_ID_COMMUNITY_LIST,
  OBJECT_ARG_TYPE_ID_IP_LIST, // IPv4 and/or IPv6
  OBJECT_ARG_TYPE_ID_IPV6_LIST,
  OBJECT_ARG_TYPE_ID_PORT_LIST,
  OBJECT_ARG_TYPE_ID_MESSAGE,
  OBJECT_ARG_TYPE_ID_PEERID_LIST, // BGP peer id
  OBJECT_ARG_TYPE_DEBUG_LEVEL,
  OBJECT_ARG_TYPE_PRBS_COMPONENT,
  OBJECT_ARG_TYPE_PRBS_STATE,
  OBJECT_ARG_TYPE_PORT_STATE,
  OBJECT_ARG_TYPE_FSDB_PATH,
};

class BaseObjectArgType {
 public:
  BaseObjectArgType() {}
  /* implicit */ BaseObjectArgType(std::vector<std::string> v) : data_(v) {}
  void validate() {}

  std::vector<std::string> data_;
  const static ObjectArgTypeId id = ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
};

class PortList : public BaseObjectArgType {
 public:
  /* implicit */ PortList(std::vector<std::string> v) : BaseObjectArgType(v) {}
  void validate() {}

  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
};

class PrbsComponent : public BaseObjectArgType {
 public:
  /* implicit */ PrbsComponent(std::vector<std::string> v)
      : BaseObjectArgType(v) {}
  void validate() {}

  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_PRBS_COMPONENT;
};

// Called after CLI11 is initlized but before parsing, for any final
// initialization steps
void postAppInit(int argc, char* argv[], CLI::App& app);

const folly::IPAddress getIPFromHost(const std::string& hostname);
const std::string getOobNameFromHost(const std::string& host);
std::vector<std::string> getHostsInSmcTier(const std::string& parentTierName);
std::vector<std::string> getHostsFromFile(const std::string& filename);
std::vector<std::string> getFsdbPath(
    const std::vector<std::string>& path,
    std::string defaultPath);
long getEpochFromDuration(const int64_t& duration);
timeval splitFractionalSecondsFromTimer(const long& timer);
const std::string parseTimeToTimeStamp(const long& timeToParse);

const std::string getPrettyElapsedTime(const int64_t& start_time);
const std::string formatBandwidth(const unsigned long& bandwidth);
std::vector<int32_t> getPortIDList(
    const std::vector<std::string>& ifList,
    std::map<int32_t, facebook::fboss::PortInfoThrift>& portEntries);

void setLogLevel(std::string logLevelStr);

void logUsage(const std::string& cmdName);

template <typename T, size_t N, size_t... Indices>
auto arrayToTupleImpl(
    const std::array<T, N>& v,
    std::index_sequence<Indices...>) {
  return std::make_tuple(v[Indices]...);
}

// convert the first N elements of an array to a tuple
template <size_t N, size_t M, typename T>
auto arrayToTuple(const std::array<T, M>& v) {
  static_assert(N <= M);
  return arrayToTupleImpl(v, std::make_index_sequence<N>());
}

// returns tuple(value) if TargetT == std::monostate, otherwise empty tuple()
template <typename TargetT, typename T>
auto tupleValueIfNotMonostate(const T& value) {
  if constexpr (std::is_same_v<TargetT, std::monostate>) {
    return std::make_tuple();
  } else {
    return std::make_tuple(value);
  }
}

template <typename UnfilteredTypes, typename Tuple, std::size_t... Indices>
auto filterTupleMonostatesImpl(Tuple tup, std::index_sequence<Indices...>) {
  return std::tuple_cat(
      tupleValueIfNotMonostate<std::tuple_element_t<Indices, UnfilteredTypes>>(
          std::get<Indices>(tup))...);
}

// Filter out all std::monostates from a tuple. the passed UnfilteredTypes
// will indicate which indices need to be removed
template <typename UnfilteredTypes, typename Tuple, std::size_t... Indices>
auto filterTupleMonostates(Tuple tup) {
  static_assert(
      std::tuple_size_v<UnfilteredTypes> == std::tuple_size_v<Tuple>,
      "Passed tuple must be the same size as passed type");
  return filterTupleMonostatesImpl<UnfilteredTypes>(
      tup, std::make_index_sequence<std::tuple_size_v<UnfilteredTypes>>());
}

} // namespace facebook::fboss::utils
