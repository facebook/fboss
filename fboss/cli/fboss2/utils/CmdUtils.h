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
#include <boost/algorithm/string/case_conv.hpp>
#include <folly/IPAddress.h>
#include <folly/String.h>
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
  using iterator = typename std::vector<std::string>::iterator;
  using const_iterator = typename std::vector<std::string>::const_iterator;
  using size_type = typename std::vector<std::string>::size_type;

  const std::vector<std::string> data() const {
    return data_;
  }

  std::string& operator[](int index) {
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

  std::vector<std::string> data_;
  const static ObjectArgTypeId id = ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
};

class CommunityList : public BaseObjectArgType {
 public:
  /* implicit */ CommunityList(std::vector<std::string> v)
      : BaseObjectArgType(v) {}

  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_ID_COMMUNITY_LIST;
};

class IPList : public BaseObjectArgType {
 public:
  /* implicit */ IPList(std::vector<std::string> v) : BaseObjectArgType(v) {}

  const static ObjectArgTypeId id = ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IP_LIST;
};

class IPV6List : public BaseObjectArgType {
 public:
  /* implicit */ IPV6List(std::vector<std::string> v) : BaseObjectArgType(v) {}

  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IPV6_LIST;
};

class PortList : public BaseObjectArgType {
 public:
  /* implicit */ PortList(std::vector<std::string> v) : BaseObjectArgType(v) {}

  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
};

class Message : public BaseObjectArgType {
 public:
  /* implicit */ Message(std::vector<std::string> v) : BaseObjectArgType(v) {}

  const static ObjectArgTypeId id = ObjectArgTypeId::OBJECT_ARG_TYPE_ID_MESSAGE;
};

class PeerIdList : public BaseObjectArgType {
 public:
  /* implicit */ PeerIdList(std::vector<std::string> v)
      : BaseObjectArgType(v) {}

  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PEERID_LIST;
};

class DebugLevel : public BaseObjectArgType {
 public:
  /* implicit */ DebugLevel(std::vector<std::string> v)
      : BaseObjectArgType(v) {}

  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_DEBUG_LEVEL;
};

class PrbsComponent : public BaseObjectArgType {
 public:
  /* implicit */ PrbsComponent(std::vector<std::string> v)
      : BaseObjectArgType(v) {}

  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_PRBS_COMPONENT;
};

class PrbsState : public BaseObjectArgType {
 public:
  /* implicit */ PrbsState(std::vector<std::string> v) : BaseObjectArgType(v) {}

  const static ObjectArgTypeId id = ObjectArgTypeId::OBJECT_ARG_TYPE_PRBS_STATE;
};

class PortState : public BaseObjectArgType {
 public:
  /* implicit */ PortState(std::vector<std::string> v) : BaseObjectArgType(v) {
    if (v.empty()) {
      throw std::runtime_error(
          "Incomplete command, expecting 'state <enable|disable>'");
    }
    if (v.size() != 1) {
      throw std::runtime_error(folly::to<std::string>(
          "Unexpected state '",
          folly::join<std::string, std::vector<std::string>>(" ", v),
          "', expecting 'enable|disable'"));
    }

    portState = getPortState(v[0]);
  }
  bool portState;
  const static ObjectArgTypeId id = ObjectArgTypeId::OBJECT_ARG_TYPE_PORT_STATE;

 private:
  bool getPortState(std::string& v) {
    auto state = boost::to_upper_copy(v);
    if (state == "ENABLE") {
      return true;
    }
    if (state == "DISABLE") {
      return false;
    }
    throw std::runtime_error(folly::to<std::string>(
        "Unexpected state '", v, "', expecting 'enable|disable'"));
  }
};

class FsdbPath : public BaseObjectArgType {
 public:
  /* implicit */ FsdbPath(std::vector<std::string> v) : BaseObjectArgType(v) {}

  const static ObjectArgTypeId id = ObjectArgTypeId::OBJECT_ARG_TYPE_FSDB_PATH;
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
