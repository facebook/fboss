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
#include <boost/algorithm/string/regex.hpp>
#include <folly/IPAddress.h>
#include <folly/String.h>
#include <folly/gen/Base.h>
#include <folly/stop_watch.h>
#include <re2/re2.h>
#include <string>
#include <variant>

#include <fboss/lib/phy/gen-cpp2/phy_types.h>
#include "fboss/agent/if/gen-cpp2/FbossCtrlAsyncClient.h"
#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/gen-cpp2/cli_types.h"
#include "fboss/cli/fboss2/utils/PrbsUtils.h"
#include "fboss/lib/phy/gen-cpp2/prbs_types.h"

namespace facebook::fboss::utils {

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
  OBJECT_ARG_TYPE_ID_PORT_LIST,
  OBJECT_ARG_TYPE_ID_MESSAGE,
  OBJECT_ARG_TYPE_ID_PEERID_LIST, // BGP peer id
  OBJECT_ARG_TYPE_ID_VIP_INJECTOR_ID,
  OBJECT_ARG_TYPE_ID_AREA_LIST,
  OBJECT_ARG_TYPE_ID_NODE_LIST,
  OBJECT_ARG_TYPE_ID_HW_OBJECT_LIST,
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
};

template <typename T>
class BaseObjectArgType {
 public:
  BaseObjectArgType() {}
  /* implicit */ BaseObjectArgType(std::vector<std::string> v) : data_(v) {}
  using iterator = typename std::vector<std::string>::iterator;
  using const_iterator = typename std::vector<std::string>::const_iterator;
  using size_type = typename std::vector<std::string>::size_type;

  const std::vector<T> data() const {
    return data_;
  }

  std::string& operator[](int index) {
    return data_[index];
  }

  const std::string& operator[](int index) const {
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

class CommunityList : public BaseObjectArgType<std::string> {
 public:
  /* implicit */ CommunityList(std::vector<std::string> v)
      : BaseObjectArgType(v) {}

  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_ID_COMMUNITY_LIST;
};

class IPList : public BaseObjectArgType<std::string> {
 public:
  /* implicit */ IPList() : BaseObjectArgType() {}
  /* implicit */ IPList(std::vector<std::string> v) : BaseObjectArgType(v) {}

  const static ObjectArgTypeId id = ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IP_LIST;
};

class IPV6List : public BaseObjectArgType<std::string> {
 public:
  /* implicit */ IPV6List(std::vector<std::string> v) : BaseObjectArgType(v) {}

  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IPV6_LIST;
};

/**
 * Whether input port name conforms to the required pattern
 * 'moduleNum/port/subport' For example, eth1/5/3 will be parsed to four parts:
 * eth(module name), 1(module number), 5(port number), 3(subport number). Error
 * will be thrown if the port name is not valid.
 */
class PortList : public BaseObjectArgType<std::string> {
 public:
  /* implicit */ PortList() : BaseObjectArgType() {}
  /* implicit */ PortList(std::vector<std::string> ports) {
    static const RE2 exp("([a-z]+)(\\d+)/(\\d+)/(\\d)");
    for (auto const& port : ports) {
      if (!RE2::FullMatch(port, exp)) {
        throw std::invalid_argument(folly::to<std::string>(
            "Invalid port name: ",
            port,
            "\nPort name must match 'moduleNum/port/subport' pattern"));
      }
    }
    // deduplicate ports while ensuring order
    std::set<std::string> uniquePorts(ports.begin(), ports.end());
    data_ = std::vector<std::string>(uniquePorts.begin(), uniquePorts.end());
  }

  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
};

/**
 * Whether input port name conforms to the required pattern
 * 'switchName:moduleNum/port/subport' For example,
 * rdsw001.n001.z004.snc1:eth1/5/3 will be parsed to five parts:
 * rdsw001.n001.z004.snc1 (switch name), eth(module name),
 * 1(module number), 5(port number), 3(subport number). Error will be
 * thrown if the port name is not valid.
 */
class SystemPortList : public BaseObjectArgType<std::string> {
 public:
  /* implicit */ SystemPortList() : BaseObjectArgType() {}
  /* implicit */ SystemPortList(const std::vector<std::string>& ports) {
    static const RE2 exp("([^:]+):([a-z]+)(\\d+)/(\\d+)/(\\d)");
    for (auto const& port : ports) {
      if (!RE2::FullMatch(port, exp)) {
        throw std::invalid_argument(folly::to<std::string>(
            "Invalid port name: ",
            port,
            "\nPort name must match 'switch:moduleNum/port/subport' pattern"));
      }
    }
    // deduplicate ports while ensuring order
    std::set<std::string> uniquePorts(ports.begin(), ports.end());
    data_ = std::vector<std::string>(uniquePorts.begin(), uniquePorts.end());
  }

  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_ID_SYSTEM_PORT_LIST;
};

class Message : public BaseObjectArgType<std::string> {
 public:
  /* implicit */ Message(std::vector<std::string> v) : BaseObjectArgType(v) {}

  const static ObjectArgTypeId id = ObjectArgTypeId::OBJECT_ARG_TYPE_ID_MESSAGE;
};

class VipInjectorID : public BaseObjectArgType<std::string> {
 public:
  /* implicit */ VipInjectorID(std::vector<std::string> v)
      : BaseObjectArgType(v) {}

  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_ID_VIP_INJECTOR_ID;
};

class PeerIdList : public BaseObjectArgType<std::string> {
 public:
  /* implicit */ PeerIdList(std::vector<std::string> v)
      : BaseObjectArgType(v) {}

  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PEERID_LIST;
};

class DebugLevel : public BaseObjectArgType<std::string> {
 public:
  /* implicit */ DebugLevel(std::vector<std::string> v)
      : BaseObjectArgType(v) {}

  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_DEBUG_LEVEL;
};

class PrbsComponent : public BaseObjectArgType<phy::PortComponent> {
 public:
  /* implicit */ PrbsComponent(std::vector<std::string> components) {
    // existing helper function from PrbsUtils.h
    data_ = prbsComponents(components, false /* returnAllIfEmpty */);
  }

  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_PRBS_COMPONENT;
};

class PhyChipType : public BaseObjectArgType<phy::DataPlanePhyChipType> {
 public:
  /* implicit */ PhyChipType(std::vector<std::string> chipTypes) {
    if (chipTypes.empty()) {
      iphyIncluded = true;
      xphyIncluded = true;
    } else {
      if (std::find(chipTypes.begin(), chipTypes.end(), "asic") !=
          chipTypes.end()) {
        iphyIncluded = true;
      }
      if (std::find(chipTypes.begin(), chipTypes.end(), "xphy") !=
          chipTypes.end()) {
        xphyIncluded = true;
      }
    }
  }

  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_PHY_CHIP_TYPE;
  bool iphyIncluded{false};
  bool xphyIncluded{false};
};

class PrbsState : public BaseObjectArgType<std::string> {
 public:
  /* implicit */ PrbsState(std::vector<std::string> args)
      : BaseObjectArgType(args) {
    if (args.empty()) {
      throw std::invalid_argument(
          "Incomplete command, expecting state <PRBSXX>");
    }
    auto state = folly::gen::from(args) |
        folly::gen::mapped([](const std::string& s) {
                   return boost::to_upper_copy(s);
                 }) |
        folly::gen::as<std::vector>();
    enabled = (state[0] != "OFF");
    if (enabled) {
      polynomial = apache::thrift::util::enumValueOrThrow<
          facebook::fboss::prbs::PrbsPolynomial>(state[0]);
    }
    if (state.size() <= 1) {
      // None of the generator or checker args are passed, therefore set both
      // the generator and checker
      generator = enabled;
      checker = enabled;
    } else {
      std::unordered_set<std::string> stateSet(state.begin() + 1, state.end());
      if (stateSet.find("GENERATOR") != stateSet.end()) {
        generator = enabled;
      }
      if (stateSet.find("CHECKER") != stateSet.end()) {
        checker = enabled;
      }
    }
  }

  bool enabled;
  facebook::fboss::prbs::PrbsPolynomial polynomial;
  std::optional<bool> generator;
  std::optional<bool> checker;
  const static ObjectArgTypeId id = ObjectArgTypeId::OBJECT_ARG_TYPE_PRBS_STATE;
};

class PortState : public BaseObjectArgType<std::string> {
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

class FsdbPath : public BaseObjectArgType<std::string> {
 public:
  /* implicit */ FsdbPath(std::vector<std::string> fsdbPath) {
    if (fsdbPath.size() > 1) {
      throw std::runtime_error(
          "Argument for fsdb path needs to be a single string starting with /, path items separated by /\n"
          "Slashes inside path tokens such as eth2/1/1 need to be escaped"
          "For example \"/agent/switchState/ports/eth2\\/1\\/1\"");
    } else if (fsdbPath.size() == 1) {
      auto pathString = fsdbPath[0];
      // Split by SLASH, ignoring ones escaped by a BACK_SLASH
      boost::split_regex(
          fsdbPath, pathString, boost::basic_regex("(?<!\\\\)/"));
      folly::gen::from(fsdbPath) |
          // prefix SLASH is enforced to differentiate b/w path and subcommand
          // names, split will give an extranous empty string at the start of
          // path so filter that out
          folly::gen::filter([](const std::string& in) { return in != ""; }) |
          // unescape strings for things like eth2\/1\/1
          folly::gen::map([](const std::string& in) {
            auto strCopy = in;
            const RE2 re("\\\\/");
            RE2::GlobalReplace(&strCopy, re, "/");
            return strCopy;
          }) |
          folly::gen::appendTo(data_);
      // TODO: validate path matches model here
    }
  }
  const static ObjectArgTypeId id = ObjectArgTypeId::OBJECT_ARG_TYPE_FSDB_PATH;
};

class FsdbClientId : public BaseObjectArgType<std::string> {
 public:
  /* implicit */ FsdbClientId(std::vector<std::string> fsdbClientId)
      : BaseObjectArgType(fsdbClientId) {
    // if there is no input, the default value will be given inside each command
  }

  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_FSDB_CLIENT_ID;
};

class HwObjectList : public BaseObjectArgType<HwObjectType> {
 public:
  /* implicit */ HwObjectList() : BaseObjectArgType() {}
  /* implicit */ HwObjectList(std::vector<std::string> hwObjectTypeArgList) {
    for (auto const& hwObjectTypeArg : hwObjectTypeArgList) {
      HwObjectType hwObjectType =
          apache::thrift::util::enumValueOrThrow<HwObjectType>(hwObjectTypeArg);
      data_.push_back(hwObjectType);
    }
  }

  const static ObjectArgTypeId id =
      ObjectArgTypeId::OBJECT_ARG_TYPE_ID_HW_OBJECT_LIST;
};

// Called after CLI11 is initlized but before parsing, for any final
// initialization steps
void postAppInit(int argc, char* argv[], CLI::App& app);

const folly::IPAddress getIPFromHost(const std::string& hostname);
const std::string getOobNameFromHost(const std::string& host);
std::vector<std::string> getHostsInSmcTier(const std::string& parentTierName);
std::vector<std::string> getHostsFromFile(const std::string& filename);
long getEpochFromDuration(const int64_t& duration);
timeval splitFractionalSecondsFromTimer(const long& timer);
const std::string parseTimeToTimeStamp(const long& timeToParse);

const std::string getPrettyElapsedTime(const int64_t& start_time);
const std::string getDurationStr(folly::stop_watch<>& watch);
const std::string formatBandwidth(const float bandwidthBytesPerSecond);
std::vector<int32_t> getPortIDList(
    const std::vector<std::string>& ifList,
    std::map<int32_t, facebook::fboss::PortInfoThrift>& portEntries);
std::string getUserInfo();
std::string getAddrStr(network::thrift::BinaryAddress addr);
std::string getAdminDistanceStr(AdminDistance adminDistance);
const std::string removeFbDomains(const std::string& host);
std::string getSpeedGbps(int64_t speedMbps);
std::string getl2EntryTypeStr(L2EntryType l2EntryType);
void setLogLevel(std::string logLevelStr);

void logUsage(const CmdLogInfo& cmdLogInfo);

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

/**
 * Whether the first port name is smaller than or equal to the second port
 * name. For example, eth1/5/3 will be parsed to four parts: eth(module name),
 * 1(module number), 5(port number), 3(subport number). The two number will
 * first be compared by the alphabetical order of their module names. If the
 * module namea are the same, then the order will be decied by the numerical
 * values of their module number, port number, subport number in order.
 * Therefore, eth420/5/1 comes before fab3/9/1 as the module name of the
 * former one is alphabetically smaller. However, eth420/5/1 comes after
 * eth1/5/3 as the module number 420 is larger than 1. Accordingly, eth1/5/3
 * comes after eth1/5/1 as its subport is larger. With input array
 * ["eth1/5/1", "eth1/5/2", "eth1/5/3", "fab402/20/1", "eth1/10/1",
 * "eth1/4/1"], the expected returned order of this comparator will be
 * ["eth1/4/1", "eth1/5/1", "eth1/5/2", "eth1/5/3",  "eth1/10/1",
 * "fab402/20/1"].
 * @param nameA    The first port name
 * @param nameB    The second port name
 * @return         true if the first port name is smaller or equal to the
 * second port name
 */
bool comparePortName(
    const std::basic_string<char>& nameA,
    const std::basic_string<char>& nameB);

bool compareSystemPortName(
    const std::basic_string<char>& nameA,
    const std::basic_string<char>& nameB);
} // namespace facebook::fboss::utils
