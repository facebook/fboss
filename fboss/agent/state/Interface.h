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

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>
#include <folly/Range.h>
#include <folly/dynamic.h>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_map.hpp>

namespace facebook::fboss {

class SwitchState;

struct InterfaceFields {
  typedef boost::container::flat_map<folly::IPAddress, uint8_t> Addresses;

  InterfaceFields(
      InterfaceID id,
      RouterID router,
      VlanID vlan,
      folly::StringPiece name,
      folly::MacAddress mac,
      int mtu,
      bool isVirtual,
      bool isStateSyncDisabled)
      : id(id),
        routerID(router),
        vlanID(vlan),
        name(name.data(), name.size()),
        mac(mac),
        mtu(mtu),
        isVirtual(isVirtual),
        isStateSyncDisabled(isStateSyncDisabled) {}

  /*
   * Deserialize from a folly::dynamic object
   */
  static InterfaceFields fromFollyDynamic(const folly::dynamic& json);
  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const;

  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}

  const InterfaceID id;
  RouterID routerID;
  VlanID vlanID;
  std::string name;
  folly::MacAddress mac;
  Addresses addrs;
  cfg::NdpConfig ndp;
  int mtu;
  bool isVirtual{false};
  bool isStateSyncDisabled{false};
};

/*
 * Interface stores a routing domain on the switch
 */
class Interface : public NodeBaseT<Interface, InterfaceFields> {
 public:
  typedef InterfaceFields::Addresses Addresses;

  Interface(
      InterfaceID id,
      RouterID router,
      VlanID vlan,
      folly::StringPiece name,
      folly::MacAddress mac,
      int mtu,
      bool isVirtual,
      bool isStateSyncDisabled)
      : NodeBaseT(
            id,
            router,
            vlan,
            name,
            mac,
            mtu,
            isVirtual,
            isStateSyncDisabled) {}

  static std::shared_ptr<Interface> fromFollyDynamic(
      const folly::dynamic& json) {
    const auto& fields = InterfaceFields::fromFollyDynamic(json);
    return std::make_shared<Interface>(fields);
  }

  static std::shared_ptr<Interface> fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  InterfaceID getID() const {
    return getFields()->id;
  }

  RouterID getRouterID() const {
    return getFields()->routerID;
  }
  void setRouterID(RouterID id) {
    writableFields()->routerID = id;
  }

  VlanID getVlanID() const {
    return getFields()->vlanID;
  }
  void setVlanID(VlanID id) {
    writableFields()->vlanID = id;
  }

  int getMtu() const {
    return getFields()->mtu;
  }
  void setMtu(int mtu) {
    writableFields()->mtu = mtu;
  }

  const std::string& getName() const {
    return getFields()->name;
  }
  void setName(const std::string& name) {
    writableFields()->name = name;
  }

  folly::MacAddress getMac() const {
    return getFields()->mac;
  }
  void setMac(folly::MacAddress mac) {
    writableFields()->mac = mac;
  }

  bool isVirtual() const {
    return getFields()->isVirtual;
  }
  void setIsVirtual(bool isVirtual) {
    writableFields()->isVirtual = isVirtual;
  }

  bool isStateSyncDisabled() const {
    return getFields()->isStateSyncDisabled;
  }
  void setIsStateSyncDisabled(bool isStateSyncDisabled) {
    writableFields()->isStateSyncDisabled = isStateSyncDisabled;
  }

  const Addresses& getAddresses() const {
    return getFields()->addrs;
  }
  void setAddresses(Addresses addrs) {
    writableFields()->addrs.swap(addrs);
  }
  bool hasAddress(folly::IPAddress ip) const {
    const auto& addrs = getAddresses();
    return addrs.find(ip) != addrs.end();
  }

  /**
   * Find the interface IP address to reach the given destination
   */
  Addresses::const_iterator getAddressToReach(
      const folly::IPAddress& dest) const;

  /**
   * Returns true if the address can be reached through this interface
   */
  bool canReachAddress(const folly::IPAddress& dest) const;

  const cfg::NdpConfig& getNdpConfig() const {
    return getFields()->ndp;
  }
  void setNdpConfig(const cfg::NdpConfig& ndp) {
    writableFields()->ndp = ndp;
  }

  /*
   * A utility function to check if an IP address is in a locally attached
   * subnet on the given interface.
   *
   * This a generic helper function.  The Interface class simply seemed like
   * the best place to put it for now.
   */
  static bool isIpAttached(
      folly::IPAddress ip,
      InterfaceID intfID,
      const std::shared_ptr<SwitchState>& state);

  /*
   * Inherit the constructors required for clone().
   * This needs to be public, as std::make_shared requires
   * operator new() to be available.
   */
  inline static const int kDefaultMtu{1500};

 private:
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
