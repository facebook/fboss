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
#include <boost/iterator/filter_iterator.hpp>

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/Range.h>
#include <folly/lang/Bits.h>

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/types.h"

namespace folly {
struct dynamic;
}

namespace facebook::fboss {

class SwitchState;

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
 * Helper function to get an IPv6 address for a particular vlan
 * used to set src IP address for DHCPv6 and ICMPv6 packets
 * throw an FbossError in case no IPv6 address exists.
 */
folly::IPAddressV6 getSwitchVlanIPv6(
    const std::shared_ptr<SwitchState>& state,
    VlanID vlan);
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

} // namespace facebook::fboss
