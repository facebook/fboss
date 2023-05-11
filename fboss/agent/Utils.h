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

#include <string>
#include <type_traits> // To use 'std::integral_constant'.

#include <boost/container/flat_map.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/iterator/filter_iterator.hpp>

#include <folly/FileUtil.h>
#include <folly/IPAddress.h>
#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/Range.h>
#include <folly/lang/Bits.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/HwSwitchMatcher.h"

#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <chrono>

DECLARE_string(mac);
DECLARE_uint64(ingress_egress_buffer_pool_size);
DECLARE_bool(allow_zero_headroom_for_lossless_pg);
namespace folly {
struct dynamic;
}

namespace facebook::fboss {

class SwitchState;
class Interface;

template <typename T>
inline T readBuffer(const uint8_t* buffer, uint32_t pos, size_t buffSize) {
  CHECK_LE(pos + sizeof(T), buffSize);
  T tmp;
  memcpy(&tmp, buffer + pos, sizeof(tmp));
  return folly::Endian::big(tmp);
}

/*
 * Create a directory, recursively creating all parents if necessary.
 *
 * If the directory already exists this function is a no-op.
 * Throws an exception on error.
 */
void utilCreateDir(folly::StringPiece path);

/**
 * Helper function to get an IPv4 address for a particular vlan
 * Used to set src IP address for DHCP and ICMP packets
 * throw an FbossError in case no IP address exists.
 */
folly::IPAddressV4 getSwitchVlanIP(
    const std::shared_ptr<SwitchState>& state,
    VlanID vlan);

/**
 * Helper function to get an IPv4 address for a particular interface
 * Used to set src IP address for DHCP and ICMP packets
 * throw an FbossError in case no IP address exists.
 */
folly::IPAddressV4 getSwitchIntfIP(
    const std::shared_ptr<SwitchState>& state,
    InterfaceID intfID);

/**
 * Helper function to get an IPv4 address of any(first) interface.
 */
folly::IPAddressV4 getAnyIntfIP(const std::shared_ptr<SwitchState>& state);

/**
 * Helper function to get an IPv6 address for a particular vlan
 * used to set src IP address for DHCPv6 and ICMPv6 packets
 * throw an FbossError in case no IPv6 address exists.
 */
folly::IPAddressV6 getSwitchVlanIPv6(
    const std::shared_ptr<SwitchState>& state,
    VlanID vlan);

/**
 * Helper function to get an IPv6 address for a particular interface
 * used to set src IP address for DHCPv6 and ICMPv6 packets
 * throw an FbossError in case no IPv6 address exists.
 */
folly::IPAddressV6 getSwitchIntfIPv6(
    const std::shared_ptr<SwitchState>& state,
    InterfaceID intfID);

/**
 * Helper function to get an IPvv6 address of any(first) interface.
 */
folly::IPAddressV6 getAnyIntfIPv6(const std::shared_ptr<SwitchState>& state);

/*
 * Increases the nice value of the calling thread by increment. Note that this
 * code relies on the fact that Linux is not POSIX compliant. Otherwise, there
 * is no way to set per-thread priorities without root privileges.
 *
 * @param[in]    increment       How much to add to the current nice value. Must
 *                               be positive because we don't have the
 *                               privileges to decrease our niceness.
 */
void incNiceValue(const uint32_t increment);

/*
 * Serialize folly dynamic to JSON and write to file
 */
bool dumpStateToFile(const std::string& filename, const folly::dynamic& json);

/*
 * Whether thrift state is valid for sw switch state recovery
 */
bool isValidThriftStateFile(
    const std::string& follyStateFileName,
    const std::string& thriftStateFileName);

std::vector<ClientID> AllClientIDs();

/*
 * Report our hostname
 */
std::string getLocalHostname();

void initThread(folly::StringPiece name);

/*
 * Utility wrapper around filter iterator
 */
template <
    typename Key,
    typename Value,
    typename Map = boost::container::flat_map<Key, Value>>
class MapFilter {
 public:
  using Entry = std::pair<Key, Value>;
  using Predicate = std::function<bool(const Entry&)>;
  using Iterator = typename Map::const_iterator;
  using FilterIterator = boost::iterators::filter_iterator<Predicate, Iterator>;

  MapFilter(const Map& map, Predicate pred)
      : map_(map), pred_(std::move(pred)) {}

  FilterIterator begin() {
    return boost::make_filter_iterator(pred_, map_.begin(), map_.end());
  }

  FilterIterator end() {
    return boost::make_filter_iterator(pred_, map_.end(), map_.end());
  }

 private:
  const Map& map_;
  Predicate pred_;
};

std::string switchRunStateStr(SwitchRunState runState);

folly::MacAddress getLocalMacAddress();

std::vector<NextHopThrift> thriftNextHopsFromAddresses(
    const std::vector<network::thrift::BinaryAddress>& addrs);

IpPrefix toIpPrefix(const folly::CIDRNetwork& nw);

UnicastRoute makeDropUnicastRoute(
    const folly::CIDRNetwork& nw,
    AdminDistance admin = AdminDistance::EBGP);
UnicastRoute makeToCpuUnicastRoute(
    const folly::CIDRNetwork& nw,
    AdminDistance admin = AdminDistance::EBGP);
UnicastRoute makeUnicastRoute(
    const folly::CIDRNetwork& nw,
    const std::vector<folly::IPAddress>& nhops,
    AdminDistance admin = AdminDistance::EBGP);

bool isAnyInterfacePortInLoopbackMode(
    std::shared_ptr<SwitchState> swState,
    const std::shared_ptr<Interface> interface);

PortID getPortID(
    SystemPortID sysPortId,
    const std::shared_ptr<SwitchState>& state);

SystemPortID getSystemPortID(
    const PortID& portId,
    const std::shared_ptr<SwitchState>& state);

std::vector<PortID> getPortsForInterface(
    InterfaceID intf,
    const std::shared_ptr<SwitchState>& state);

class StopWatch {
 public:
  StopWatch(std::optional<std::string> name, bool json);
  ~StopWatch();

  std::chrono::duration<double, std::milli> msecsElapsed() const {
    return std::chrono::steady_clock::now() - startTime_;
  }

 private:
  const std::optional<std::string> name_;
  bool json_;
  std::chrono::time_point<std::chrono::steady_clock> startTime_;
};

inline constexpr uint8_t kGetNetworkControlTrafficClass() {
  // Network Control << ECN-bits
  return 48 << 2;
}

void enableExactMatch(std::string& yamlCfg);

/*
 * Serialize thrift struct to binary and write to file
 */
template <typename ThriftT>
bool dumpBinaryThriftToFile(
    const std::string& filename,
    const ThriftT& thrift) {
  // create parent path if doesn't exist
  utilCreateDir(boost::filesystem::path(filename).parent_path().string());
  apache::thrift::BinaryProtocolWriter writer;
  folly::IOBufQueue queue;
  writer.setOutput(&queue);
  auto bytesWritten = thrift.write(&writer);
  std::vector<std::byte> result;
  result.resize(bytesWritten);
  // @lint-ignore CLANGTIDY
  folly::io::Cursor(queue.front()).pull(result.data(), bytesWritten);
  return folly::writeFile(result, filename.c_str());
}

/*
 * Deserialize thrift struct to from file and write into thriftState
 */
template <typename ThriftT>
bool readThriftFromBinaryFile(
    const std::string& filename,
    ThriftT& thriftState) {
  std::vector<std::byte> serializedThrift;
  auto ret = folly::readFile(filename.c_str(), serializedThrift);
  if (ret) {
    auto buf = folly::IOBuf::copyBuffer(
        serializedThrift.data(), serializedThrift.size());
    apache::thrift::BinaryProtocolReader reader;
    reader.setInput(buf.get());
    thriftState.read(&reader);
    return true;
  }
  return false;
}
/*
 * Helper function to get neighbor entry for specified IP.
 *
 * for VLAN based interface, look up the neighbor table for VLAN.
 * for Port based interface, look up the neighbor table for interface.
 */
template <typename NeighborEntryT>
std::shared_ptr<NeighborEntryT> getNeighborEntryForIP(
    const std::shared_ptr<SwitchState>& state,
    const std::shared_ptr<Interface>& intf,
    const folly::IPAddress& ipAddr);

class OperDeltaFilter {
 public:
  explicit OperDeltaFilter(SwitchID switchId);
  std::optional<fsdb::OperDelta> filterWithSwitchStateRootPath(
      const fsdb::OperDelta& delta) const {
    return filter(delta, 1);
  }

  std::optional<fsdb::OperDelta> filter(const fsdb::OperDelta& delta, int index)
      const;

 private:
  SwitchID switchId_;
  mutable std::map<std::string, HwSwitchMatcher> matchersCache_;
};

} // namespace facebook::fboss
