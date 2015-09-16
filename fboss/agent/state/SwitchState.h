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

#include <chrono>
#include <memory>

#include <folly/FBString.h>
#include <folly/dynamic.h>

#include "fboss/agent/types.h"
#include "fboss/agent/state/NodeBase.h"

namespace facebook { namespace fboss {

class Port;
class PortMap;
class Vlan;
class VlanMap;
class Interface;
class InterfaceMap;
class RouteTable;
class RouteTableMap;

struct SwitchStateFields {
  SwitchStateFields();

  template<typename Fn>
  void forEachChild(Fn fn) {
    fn(ports.get());
    fn(vlans.get());
    fn(interfaces.get());
    fn(routeTables.get());
  }
  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const;
  /*
   * Reconstruct object from folly::dynamic
   */
  static SwitchStateFields fromFollyDynamic(const folly::dynamic& json);
  // Static state, which can be accessed without locking.
  std::shared_ptr<PortMap> ports;
  std::shared_ptr<VlanMap> vlans;
  std::shared_ptr<InterfaceMap> interfaces;
  std::shared_ptr<RouteTableMap> routeTables;
  VlanID defaultVlan{0};

  // Timeout settings
  // TODO(aeckert): Figure out a nicer way to store these config fields
  // in an accessible way
  std::chrono::seconds arpTimeout{60};
  std::chrono::seconds arpAgerInterval{5};
};

/*
 * SwitchState stores the current switch configuration.
 *
 * The SwitchState stores two primary types of data: static data and dynamic
 * data.
 *
 * Static data changes infrequently.  This is generally data that is only
 * changed by human interaction, such as data loaded from the configuration
 * file.  This includes things like port configuration and VLAN configuration.
 *
 * Dynamic data may change more frequently.  This is generally data that is
 * updated automatically, and/or learned from the network.  This includes
 * things like ARP entries.
 *
 *
 * The SwitchState manages static and dynamic data differently.  Static data is
 * read-only within the SwitchState object.  This allows it to be read without
 * locking.  When it does need to be changed, a new copy of the SwitchState is
 * made, and the SwSwitch is updated to point to the new version.  To access
 * static data, callers need to atomically load the current SwitchState from
 * the SwSwitch, but from then on they do not need any further locking or
 * atomic operations.
 *
 * When performing copy-on-write, the SwitchState only copies portions of the
 * state that are actually modified.  Parts of the state that are unchanged
 * will be re-used by the new SwitchState object.
 *
 *
 * Because dynamic data changes more frequently, it is allowed to be modified
 * without making a new copy-on-write version of the SwitchState.  Therefore
 * locks are used to protect access to dynamic data.
 */
class SwitchState : public NodeBaseT<SwitchState, SwitchStateFields> {
 public:
  /*
   * Create a new, empty state.
   */
  SwitchState();
  ~SwitchState() override;

  static std::shared_ptr<SwitchState>
  fromFollyDynamic(const folly::dynamic& json) {
    const auto& fields = SwitchStateFields::fromFollyDynamic(json);
    return std::make_shared<SwitchState>(fields);
  }

  static std::shared_ptr<SwitchState>
  fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  static void modify(std::shared_ptr<SwitchState>* state);

  const std::shared_ptr<PortMap>& getPorts() const {
    return getFields()->ports;
  }
  std::shared_ptr<Port> getPort(PortID id) const;

  const std::shared_ptr<VlanMap>& getVlans() const {
    return getFields()->vlans;
  }

  VlanID getDefaultVlan() const {
    return getFields()->defaultVlan;
  }
  void setDefaultVlan(VlanID id);

  const std::shared_ptr<InterfaceMap>& getInterfaces() const {
    return getFields()->interfaces;
  }
  const std::shared_ptr<RouteTableMap>& getRouteTables() const {
    return getFields()->routeTables;
  }

  std::chrono::seconds getArpTimeout() const {
    return getFields()->arpTimeout;
  }

  void setArpTimeout(std::chrono::seconds timeout);

  std::chrono::seconds getArpAgerInterval() const {
    return getFields()->arpAgerInterval;
  }

  void setArpAgerInterval(std::chrono::seconds interval);

  /*
   * The following functions modify the static state.
   * The should only be called on newly created SwitchState objects that are
   * only visible to a single thread, before they are published as the current
   * state.
   */

  void registerPort(PortID id, const std::string& name);
  void resetPorts(std::shared_ptr<PortMap> ports);
  void resetVlans(std::shared_ptr<VlanMap> vlans);
  void addVlan(const std::shared_ptr<Vlan>& vlan);
  void addIntf(const std::shared_ptr<Interface>& intf);
  void resetIntfs(std::shared_ptr<InterfaceMap> intfs);
  void addRouteTable(const std::shared_ptr<RouteTable>& rt);
  void resetRouteTables(std::shared_ptr<RouteTableMap> rts);

 private:
  // Inherit the constructor required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

}} // facebook::fboss
