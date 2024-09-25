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
#include <folly/stop_watch.h>
#include <string>
#include <variant>

namespace facebook::fboss::utils {

struct LocalOption {
  std::string name;
  std::string helpMsg;
};

struct CmdLogInfo {
  std::string CmdName;
  std::string Duration;
  std::string Arguments;
  std::string UserInfo;
  std::string ExitStatus;
};

enum class ObjectArgTypeId : uint8_t {
  OBJECT_ARG_TYPE_ID_UNINITIALIZE = 0,
  OBJECT_ARG_TYPE_ID_NONE,
  OBJECT_ARG_TYPE_ID_COMMUNITY_LIST,
  OBJECT_ARG_TYPE_ID_IP_LIST, // IPv4 and/or IPv6
  OBJECT_ARG_TYPE_ID_IPV6_LIST,
  OBJECT_ARG_TYPE_ID_CIDR_NETWORK,
  OBJECT_ARG_TYPE_ID_PORT_LIST,
  OBJECT_ARG_TYPE_ID_MESSAGE,
  OBJECT_ARG_TYPE_ID_PEERID_LIST, // BGP peer id
  OBJECT_ARG_TYPE_ID_VIP_INJECTOR_ID,
  OBJECT_ARG_TYPE_ID_AREA_LIST,
  OBJECT_ARG_TYPE_ID_NODE_LIST,
  OBJECT_ARG_TYPE_ID_HW_OBJECT_LIST,
  OBJECT_ARG_TYPE_ID_SWITCH_NAME_LIST,
  OBJECT_ARG_TYPE_DEBUG_LEVEL,
  OBJECT_ARG_TYPE_PRBS_COMPONENT,
  OBJECT_ARG_TYPE_PRBS_STATE,
  OBJECT_ARG_TYPE_PORT_STATE,
  OBJECT_ARG_TYPE_FSDB_PATH,
  OBJECT_ARG_TYPE_AS_SEQUENCE,
  OBJECT_ARG_TYPE_LOCAL_PREFERENCE,
  OBJECT_ARG_TYPE_PHY_CHIP_TYPE,
  OBJECT_ARG_TYPE_FSDB_CLIENT_ID,
  OBJECT_ARG_TYPE_ID_SYSTEM_PORT_LIST,
  OBJECT_ARG_TYPE_ID_MIRROR_LIST,
  OBJECT_ARG_TYPE_LINK_DIRECTION,
  OBJECT_ARG_TYPE_FAN_PWM,
};

template <typename T>
class BaseObjectArgType {
 public:
  BaseObjectArgType() {}
  /* implicit */ BaseObjectArgType(std::vector<T> v) : data_(v) {}
  using iterator = typename std::vector<T>::iterator;
  using const_iterator = typename std::vector<T>::const_iterator;
  using size_type = typename std::vector<T>::size_type;

  const std::vector<T> data() const {
    return data_;
  }

  T& operator[](int index) {
    return data_[index];
  }

  const T& operator[](int index) const {
    return data_[index];
  }
  iterator begin() {
    return data_.begin();
  }
  iterator end() {
    return data_.end();
  }
  const_iterator begin() const {
    return data_.begin();
  }
  const_iterator end() const {
    return data_.end();
  }

  size_type size() const {
    return data_.size();
  }

  bool empty() const {
    return data_.empty();
  }

  std::vector<T> data_;
  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_ID_UNINITIALIZE;
};

class NoneArgType : public BaseObjectArgType<std::string> {
 public:
  /* implicit */ NoneArgType(std::vector<std::string> v)
      : BaseObjectArgType(v) {}

  const static ObjectArgTypeId id = ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
};

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
  // backward compatibility
  if constexpr (std::is_same_v<TargetT, std::monostate>) {
    return std::make_tuple();
  }
  // NoneArgType indicates OBJECT_ARG_TYPE_ID_NONE
  else if constexpr (std::is_same_v<TargetT, NoneArgType>) {
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

// Called after CLI11 is initlized but before parsing, for any final
// initialization steps
void postAppInit(int argc, char* argv[], CLI::App& app);

// API to retrieve host related information
std::pair<std::string, folly::IPAddress> getCanonicalNameAndIPFromHost(
    const std::string& hostname);
const std::string getOobNameFromHost(const std::string& host);
std::vector<std::string> getHostsInSmcTier(const std::string& parentTierName);
std::vector<std::string> getHostsFromFile(const std::string& filename);

// Common util method
timeval splitFractionalSecondsFromTimer(const long& timer);
const std::string parseTimeToTimeStamp(const long& timeToParse);
const std::string formatBandwidth(const float bandwidthBytesPerSecond);
long getEpochFromDuration(const int64_t& duration);
const std::string getDurationStr(folly::stop_watch<>& watch);
const std::string getPrettyElapsedTime(const int64_t& start_time);

std::string getUserInfo();
std::string getUnixname();

void setLogLevel(const std::string& logLevelStr);
void logUsage(const CmdLogInfo& cmdLogInfo);

} // namespace facebook::fboss::utils
