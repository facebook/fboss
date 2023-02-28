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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/types.h"

#include <sstream>

namespace {
constexpr auto kIpAddr = "ipaddress";
constexpr auto kMac = "mac";
constexpr auto kNeighborPort = "portId";
constexpr auto kInterface = "interfaceId";
constexpr auto kNeighborEntryState = "state";
constexpr auto kClassID = "classID";
constexpr auto kEncapIndex = "encapIndex";
constexpr auto kIsLocal = "isLocal";
} // namespace

namespace facebook::fboss {

template <typename IPADDR, typename SUBCLASS>
NeighborEntry<IPADDR, SUBCLASS>::NeighborEntry(
    AddressType ip,
    folly::MacAddress mac,
    PortDescriptor port,
    InterfaceID intfID,
    NeighborState state,
    std::optional<cfg::AclLookupClass> classID,
    std::optional<int64_t> encapIndex,
    bool isLocal) {
  this->setIP(ip);
  this->setMAC(mac);
  this->setPort(port);
  this->setIntfID(intfID);
  this->setState(state);
  this->setClassID(classID);
  this->setEncapIndex(encapIndex);
  this->setIsLocal(isLocal);
}

template <typename IPADDR, typename SUBCLASS>
NeighborEntry<IPADDR, SUBCLASS>::NeighborEntry(
    AddressType ip,
    InterfaceID interfaceID,
    NeighborState pending,
    std::optional<int64_t> encapIndex,
    bool isLocal) {
  CHECK(pending == NeighborState::PENDING);

  this->setIP(ip);
  this->setIntfID(interfaceID);
  this->setState(pending);
  this->setEncapIndex(encapIndex);
  this->setIsLocal(isLocal);

  /* default */
  this->setMAC(MacAddress::BROADCAST);
  this->setPort(PortDescriptor(PortID(0)));
}

template <typename IPADDR, typename SUBCLASS>
std::string NeighborEntry<IPADDR, SUBCLASS>::str() const {
  std::ostringstream os;

  auto classIDStr = getClassID().has_value()
      ? folly::to<std::string>(static_cast<int>(getClassID().value()))
      : "None";
  auto neighborStateStr =
      isReachable() ? "Reachable" : (isPending() ? "Pending" : "Unverified");

  auto encapStr = getEncapIndex().has_value()
      ? folly::to<std::string>(getEncapIndex().value())
      : "None";
  os << "NeighborEntry:: MAC: " << getMac().toString()
     << " IP: " << getIP().str() << " classID: " << classIDStr << " "
     << " Encap index: " << encapStr
     << " isLocal: " << (getIsLocal() ? "Y" : "N")
     << " Port: " << getPort().str() << " NeighborState: " << neighborStateStr;

  return os.str();
}

} // namespace facebook::fboss
