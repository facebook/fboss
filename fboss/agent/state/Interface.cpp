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

#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <optional>
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/SwitchState.h"
#include "folly/IPAddress.h"
#include "folly/MacAddress.h"

using folly::IPAddress;
using folly::MacAddress;
using folly::to;
using std::string;

namespace {
constexpr auto kInterfaceId = "interfaceId";
constexpr auto kRouterId = "routerId";
constexpr auto kVlanId = "vlanId";
constexpr auto kName = "name";
constexpr auto kMac = "mac";
constexpr auto kAddresses = "addresses";
constexpr auto kNdpConfig = "ndpConfig";
constexpr auto kMtu = "mtu";
constexpr auto kIsVirtual = "isVirtual";
constexpr auto kIsStateSyncDisabled = "isStateSyncDisabled";
} // namespace

namespace facebook::fboss {

std::optional<folly::CIDRNetwork> Interface::getAddressToReach(
    const folly::IPAddress& dest) const {
  auto getAddressToReachFn = [this, &dest](auto addresses) {
    std::optional<folly::CIDRNetwork> reachableBy;
    for (const auto& [ipStr, mask] : addresses) {
      auto cidr = folly::CIDRNetwork(ipStr, mask);
      if (dest.inSubnet(cidr.first, cidr.second)) {
        reachableBy = cidr;
        break;
      }
    }
    return reachableBy;
  };

  /*
   * Preserving old behavior for mitigation of S289408
   * Prior to migrating InterfaceFields to native thrift
   * representation, we maintained interface addresses in
   * flat_map<IPAddress, uint16_t> data structure. That is
   * these were sorted based on IP address. In thrift
   * representation we store them in a std::map<string, uint16_t>
   * i.e. sorted by string.
   * This has some unfortunate downstream implications for
   * server subnets. Consider, for server facing subnet
   * our downlink interface (vlan2000) has 2 link local
   * addresses - fe80::face:b00c and another LL derived
   * from our own mac address. Only one of these LLs is used
   * in neighbor solicitations sent from RSW to servers.
   * Which one we use depends on which occurs first in
   * the address storage data structure. Prior to
   * thrift migration, fe80::face:b00c always came
   * before dynamic LL, while with string comparison
   * dynamic LL comes first.
   * The source address RSW uses for neighbor solicitations
   * influences behavior of neighbor entries on servers.
   * When using fe80::face:b00c, a neighbor entry for that
   * IP will be created on servers on receiving NS (neighbor
   * solicitations) from RSW.  On servers fe80::face:b00C
   * is used as gateway for V4 while RSW's dynamic LL is
   * used as gateway for V6. Since V6 traffic is always
   * flowing we are confident of RSW's dynamic LL being
   * resolved on servers. However, fe80::face:b00c needs
   * either V4 traffic or a NS using that src IP from RSW.
   * Further certain services, on startup check for
   * both GWs to be resolved. And get into a crash loop
   * otherwise. Now the following 2 conditions makes
   * these services vulnerable
   *  - Absence of V4 traffic on servers
   *  - RSW using dynamic LL for NS1
   *  The concerned services are updating their check
   *  to protect from this vulnerability. Meanwhile
   *  we are reverting to old behavior so as to not
   *  couple our release with their rollout.
   */
  if (!routerAddress()) {
    return getAddressToReachFn(getAddressesCopy());
  } else {
    /*
     * If RA router address is configured, use MAC derived LL if it can
     * reach destination address. This is to allow hitless roll back of
     * this config. Consider, w/o this config we used to use MAC derived
     * LL in RA advertisements. This address would then get used as default
     * GW address for v6 traffic and get resolved. When we start using
     * a configured address (say fe80::face:b00c), this address will no longer
     * be resolved. And if we undo this RA.routerAddress config (rollback case)
     * we may get a brief blip in connectivity while the default GW is resolved.
     */
    folly::CIDRNetwork linkLocalCidr{
        folly::IPAddressV6(folly::IPAddressV6::LINK_LOCAL, getMac()), 64};
    if (dest.inSubnet(linkLocalCidr.first, linkLocalCidr.second)) {
      return linkLocalCidr;
    }
    // No longer need special case behavior since with routerAddress configured
    // we would have unified v4 and v6 GW addresses
    return getAddressToReachFn(getAddressesCopy());
  }
  return std::nullopt;
}

bool Interface::canReachAddress(const folly::IPAddress& dest) const {
  return getAddressToReach(dest).has_value();
}

std::optional<SystemPortID> Interface::getSystemPortID() const {
  std::optional<SystemPortID> sysPort;
  if (getType() == cfg::InterfaceType::SYSTEM_PORT) {
    sysPort = SystemPortID(static_cast<int>(getID()));
  }
  return sysPort;
}

bool Interface::isIpAttached(
    folly::IPAddress ip,
    InterfaceID intfID,
    const std::shared_ptr<SwitchState>& state) {
  const auto& allIntfs = state->getInterfaces();
  const auto intf = allIntfs->getNodeIf(intfID);
  if (!intf) {
    return false;
  }

  // Verify the address is reachable through this interface
  return intf->canReachAddress(ip);
}

Interface* Interface::modify(std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }
  bool isLocal = false;

  auto switchSettings = (*state)->getSwitchSettings()->size()
      ? (*state)->getSwitchSettings()->cbegin()->second
      : std::make_shared<SwitchSettings>();

  if (getType() == cfg::InterfaceType::SYSTEM_PORT) {
    auto id(static_cast<int64_t>(getID()));
    auto switchId2Info = switchSettings->getSwitchIdToSwitchInfo();
    for (const auto& [_, switchInfo] : switchId2Info) {
      if (switchInfo.systemPortRange().has_value()) {
        auto sysPortRange = *switchInfo.systemPortRange();
        if (id >= *sysPortRange.minimum() && id <= *sysPortRange.maximum()) {
          isLocal = true;
          break;
        }
      }
    }
  } else {
    isLocal = true;
  }
  auto interfaces = isLocal ? (*state)->getInterfaces()->modify(state)
                            : (*state)->getRemoteInterfaces()->modify(state);
  auto scope = interfaces->getNodeAndScope(getID()).second;
  auto newInterface = clone();
  auto* ptr = newInterface.get();
  interfaces->updateNode(std::move(newInterface), scope);
  return ptr;
}

template class ThriftStructNode<Interface, state::InterfaceFields>;

} // namespace facebook::fboss
