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

extern "C" {
#include <opennsl/types.h>
#include <opennsl/l3.h>
}

#include <boost/container/flat_map.hpp>
#include <folly/dynamic.h>
#include <folly/IPAddress.h>
#include <set>

#include "fboss/agent/hw/bcm/BcmHostKey.h"
#include "fboss/agent/hw/bcm/BcmLabeledTunnel.h"
#include "fboss/agent/types.h"
#include "fboss/lib/RefMap.h"

namespace facebook { namespace fboss {

class Interface;
class BcmSwitch;
class BcmHost;

class BcmStation {
 public:
  // L2 station can be used to filter based on MAC, VLAN, or Port.
  // We only use it to filter based on the MAC to enable the L3 processing.
  explicit BcmStation(const BcmSwitch* hw) : hw_(hw) {}
  ~BcmStation();
  void program(folly::MacAddress mac, int id);
  static uint32_t getAdditionalFlags();

 private:
  // no copy or assignment
  BcmStation(BcmStation const&) = delete;
  BcmStation& operator=(BcmStation const&) = delete;
  enum : int {
    INVALID = -1,
  };
  const BcmSwitch* hw_;
  int id_{INVALID};
};

/**
 *  BcmIntf is the class to abstract the L3 interface in BcmSwitch
 */
class BcmIntf {
 public:
  using BcmLabeledTunnelTableKey = LabelForwardingAction::LabelStack;
  using BcmLabeledTunnelMap =
      FlatRefMap<BcmLabeledTunnelTableKey, BcmLabeledTunnel>;

  explicit BcmIntf(BcmSwitch* hw);
  virtual ~BcmIntf();
  opennsl_if_t getBcmIfId() const {
    return bcmIfId_;
  }
  const std::shared_ptr<Interface>& getInterface() const {
    return intf_;
  }
  void program(const std::shared_ptr<Interface>& intf);
  folly::dynamic toFollyDynamic() const;

  std::shared_ptr<BcmLabeledTunnel> getBcmLabeledTunnel(
      const LabelForwardingAction::LabelStack& stack);

 private:
  // no copy or assignment
  BcmIntf(BcmIntf const &) = delete;
  BcmIntf& operator=(BcmIntf const &) = delete;
  enum : opennsl_if_t {
    INVALID = -1,
  };
  BcmSwitch* hw_;
  std::shared_ptr<Interface> intf_;
  opennsl_if_t bcmIfId_{INVALID};
  // TODO: we now generate one station entry per interface, even if all
  //       interfaces are sharing the same MAC. We can save some station
  //       entries by just generate one per different MAC. But with
  //       the small number of interfaces we have, no gain for that now.
  std::unique_ptr<BcmStation> station_;
  // The interface addresses that have BcmHost object created
  std::set<BcmHostKey> hosts_;
  BcmLabeledTunnelMap map_;
};

class BcmIntfTable {
 public:
  explicit BcmIntfTable(BcmSwitch* hw);
  virtual ~BcmIntfTable();

  // throw an error if not found
  BcmIntf* getBcmIntf(InterfaceID id) const;
  BcmIntf* getBcmIntf(opennsl_if_t id) const;

  // return nullptr if not found
  BcmIntf* getBcmIntfIf(InterfaceID id) const;
  BcmIntf* getBcmIntfIf(opennsl_if_t id) const;

  // The following functions will modify the object. They rely on the global
  // HW update lock in BcmSwitch::lock_ for the protection.
  void addIntf(const std::shared_ptr<Interface>& intf);
  void programIntf(const std::shared_ptr<Interface>& intf);
  void deleteIntf(const std::shared_ptr<Interface>& intf);

  // Serialize to folly::dynamic
  folly::dynamic toFollyDynamic() const;
 private:
  BcmSwitch* hw_;
  // There are two mapping tables with different index types.
  // Both are mapped to the BcmIntf. The BcmIntf object's life is
  // controlled by table with InterfaceID as the index (intfs_).
  boost::container::flat_map<InterfaceID, std::unique_ptr<BcmIntf>> intfs_;
  boost::container::flat_map<opennsl_if_t, BcmIntf *> bcmIntfs_;
};

}} // namespace facebook::fboss
