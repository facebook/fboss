/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/Interface.h"

#include <thrift/lib/cpp/util/ThriftSerializer.h>
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/SwitchState.h"

using folly::IPAddress;
using folly::MacAddress;
using folly::to;
using std::string;
using apache::thrift::util::ThriftSerializerJson;

namespace {
constexpr auto kIntfId = "interfaceId";
constexpr auto kRouterId = "routerId";
constexpr auto kVlanId = "vlanId";
constexpr auto kName = "name";
constexpr auto kMac = "mac";
constexpr auto kAddresses = "addresses";
constexpr auto kNdpConfig = "ndpConfig";
constexpr auto kMtu = "mtu";
}

namespace facebook { namespace fboss {

InterfaceFields InterfaceFields::fromFollyDynamic(const folly::dynamic& json) {
  auto intfFields =
    InterfaceFields(
        InterfaceID(json[kIntfId].asInt()),
        RouterID(json[kRouterId].asInt()),
        VlanID(json[kVlanId].asInt()),
        json[kName].asString(),
        MacAddress(json[kMac].asString()),
        json[kMtu].asInt());
  ThriftSerializerJson<cfg::NdpConfig> serializer;
  for (const auto& addr: json[kAddresses]) {
    auto cidr = IPAddress::createNetwork(addr.asString(),
          -1 /*use /32 for v4 and /128 for v6*/,
        false /*don't apply mask*/);
    intfFields.addrs[cidr.first] = cidr.second;
  }
  serializer.deserialize(toJson(json[kNdpConfig]),
      &intfFields.ndp);
  return intfFields;
}

folly::dynamic InterfaceFields::toFollyDynamic() const {
  folly::dynamic intf = folly::dynamic::object;
  intf[kIntfId] = static_cast<uint32_t>(id);
  intf[kRouterId] = static_cast<uint32_t>(routerID);
  intf[kVlanId] = static_cast<uint16_t>(vlanID);
  intf[kName] = name;
  intf[kMac] = to<string>(mac);
  intf[kMtu] = to<string>(mtu);
  std::vector<folly::dynamic> addresses;
  for (auto const& addrAndMask: addrs) {
    addresses.emplace_back(to<string>(addrAndMask.first) + "/" +
          to<string>(addrAndMask.second));
  }
  intf[kAddresses] = folly::dynamic(addresses.begin(), addresses.end());
  ThriftSerializerJson<cfg::NdpConfig> serializer;
  string ndpCfgJson;
  serializer.serialize(ndp, &ndpCfgJson);
  intf[kNdpConfig] = folly::parseJson(ndpCfgJson);
  return intf;
}

Interface::Addresses::const_iterator Interface::getAddressToReach(
    const folly::IPAddress& dest) const {
  for (auto iter = getAddresses().begin();
       iter != getAddresses().end();
       iter ++) {
    if (dest.inSubnet(iter->first, iter->second)) {
      return iter;
    }
  }
  return getAddresses().end();
}

bool Interface::canReachAddress(const folly::IPAddress& dest) const {
  return getAddressToReach(dest) != getAddresses().end();
}

bool Interface::isIpAttached(folly::IPAddress ip,
                             InterfaceID intfID,
                             const std::shared_ptr<SwitchState>& state) {
  const auto& allIntfs = state->getInterfaces();
  const auto intf = allIntfs->getInterfaceIf(intfID);
  if (!intf) {
    return false;
  }

  // Verify the address is reachable through this interface
  return intf->canReachAddress(ip);
}

template class NodeBaseT<Interface, InterfaceFields>;

}} // facebook::fboss
