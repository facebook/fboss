/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "BcmIntf.h"

extern "C" {
#include <opennsl/l2.h>
}

#include <folly/IPAddress.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/Constants.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/state/Interface.h"

namespace facebook { namespace fboss {

using std::unique_ptr;
using std::shared_ptr;
using std::make_pair;
using folly::MacAddress;
using folly::IPAddress;

class BcmStation {
 public:
  // L2 station can be used to filter based on MAC, VLAN, or Port.
  // We only use it to filter based on the MAC to enable the L3 processing.
  explicit BcmStation(const BcmSwitch* hw) : hw_(hw) {}
  ~BcmStation();
  void program(MacAddress mac, int id);
 private:
  // no copy or assignment
  BcmStation(BcmStation const &) = delete;
  BcmStation& operator=(BcmStation const &) = delete;
  enum : int {
    INVALID = -1,
  };
  const BcmSwitch *hw_;
  int id_{INVALID};
};

BcmStation::~BcmStation() {
  if (id_ == INVALID) {
    return;
  }
  auto rc = opennsl_l2_station_delete(hw_->getUnit(), id_);
  bcmLogFatal(rc, hw_, "failed to delete station entry ", id_);
  XLOG(DBG3) << "deleted station entry " << id_;
}

void BcmStation::program(MacAddress mac, int id) {
  CHECK_EQ(INVALID, id_);
  opennsl_l2_station_t params;
  opennsl_l2_station_t_init(&params);
  memcpy(&params.dst_mac, mac.bytes(), sizeof(params.dst_mac));
  memset(&params.dst_mac_mask, 0xFF, sizeof(params.dst_mac_mask));
  params.flags |= OPENNSL_L2_STATION_IPV4|OPENNSL_L2_STATION_IPV6
    |OPENNSL_L2_STATION_ARP_RARP;
  if (id != INVALID) {
    params.flags |= OPENNSL_L2_STATION_WITH_ID;
  }
  int rc;
  bool addStation = false;
  const auto warmBootCache = hw_->getWarmBootCache();
  auto vlanStationItr = warmBootCache->findVlanStation(VlanID(id));
  if (vlanStationItr != warmBootCache->vlan2Station_end()) {
    // Lambda to compare with existing entry to check if we need to reprogram
    auto equivalent =
      [=] (const opennsl_l2_station_t& newStation,
          const opennsl_l2_station_t& oldStation) {
      return macFromBcm(oldStation.dst_mac) == macFromBcm(newStation.dst_mac) &&
        macFromBcm(oldStation.dst_mac_mask) ==
        macFromBcm(newStation.dst_mac_mask);
    };
    const auto& existingStation = vlanStationItr->second;
    if (!equivalent(params, existingStation)) {
      // Delete old station end and set addStation to true
      XLOG(DBG1) << "Updating BCM station with Mac : " << mac << " and " << id;
      rc = opennsl_l2_station_delete(hw_->getUnit(), id);
      bcmCheckError(rc, "failed to delete station entry ", id);
      addStation = true;
    } else {
      XLOG(DBG1) << " station entry " << id << " already exists ";
      id_ = id;
    }

  } else {
    addStation = true;
  }

  if (addStation) {
    if (vlanStationItr != warmBootCache->vlan2Station_end()) {
      XLOG(DBG1) << "Adding BCM station with Mac : " << mac << " and " << id;
    }
    rc = opennsl_l2_station_add(hw_->getUnit(), &id, &params);
    bcmCheckError(rc, "failed to program station entry ", id,
        " to ", mac.toString());
    id_ = id;
    XLOG(DBG1) << "updated station entry " << id_ << " to " << mac.toString();
  }
  if (vlanStationItr != warmBootCache->vlan2Station_end()) {
    warmBootCache->programmed(vlanStationItr);
  }
  CHECK_NE(id_, INVALID);
}

namespace {
auto constexpr kMtu = "mtu";
auto constexpr kIntfs = "intfs";
auto constexpr kVlan = "vlan";
}

BcmIntf::BcmIntf(const BcmSwitch *hw) : hw_(hw) {
}

void BcmIntf::program(const shared_ptr<Interface>& intf) {
  const auto& oldIntf = intf_;
  if (oldIntf != nullptr) {
    // vrf and vid cannot be changed by this method.
    if (oldIntf->getRouterID() != intf->getRouterID()
        || oldIntf->getVlanID() != intf->getVlanID()) {
      throw FbossError("Interface router ID (", oldIntf->getRouterID(), " vs ",
                       intf->getRouterID(), " or VLAN ID (",
                       oldIntf->getVlanID(), " vs " , intf->getVlanID(),
                       " is changed during programming");
    }
  }

  // create or update a station entry
  if (station_ == nullptr) {
    station_ = unique_ptr<BcmStation>(new BcmStation(hw_));
  }
  if (oldIntf == nullptr || oldIntf->getMac() != intf->getMac()) {
    // Program BCM station to have the same ID as interface
    // so we can look them up during warm boot
    station_->program(intf->getMac(), intf->getID());
  }

  const auto vrf = BcmSwitch::getBcmVrfId(intf->getRouterID());

  // Lambda to compare with existing intf to determine if we need to
  // reprogram
  auto updateIntfIfNeeded =
    [&] (opennsl_l3_intf_t& newIntf,
        const opennsl_l3_intf_t& oldIntf) {
    auto equivalent =
      [=] (const opennsl_l3_intf_t& newIntf,
          const opennsl_l3_intf_t& existingIntf) {
      CHECK(newIntf.l3a_vrf == existingIntf.l3a_vrf);
      return macFromBcm(newIntf.l3a_mac_addr) ==
        macFromBcm(existingIntf.l3a_mac_addr) &&
        newIntf.l3a_vid == existingIntf.l3a_vid
        && newIntf.l3a_mtu == existingIntf.l3a_mtu;
      };

    if (!equivalent(newIntf, oldIntf)) {
      XLOG(DBG1) << "Updating interface for vlan : " << intf->getVlanID()
                 << " and mac: " << intf->getMac();
      CHECK_NE(bcmIfId_, INVALID);
      newIntf.l3a_intf_id = bcmIfId_;
      newIntf.l3a_flags = OPENNSL_L3_WITH_ID | OPENNSL_L3_REPLACE;
      return true;
     }
      return false;
   };

  const auto intftoIfparam =
    [=] (const auto& Intf) {
      opennsl_l3_intf_t ifParams;
      opennsl_l3_intf_t_init(&ifParams);
#ifndef IS_OSS
      // Something about the open source build breaks here in ubuntu 16_04
      //
      // Specifically g++-5.4 seems to segfault (!!) compiling this line.
      // It's a simple assignment from ultimately a
      // BOOST_STRONG_TYPE RouterID (which is an int) to
      // a opennsl_vrf_t (which is also an int!!).  The casting should
      // be a NOOP but somehow kills g++'s type checking system
      // This is not a problem with g++-6 that the ONL OSS build uses
      //
      // -- hack around this until compilers fix themselves :-/
      //
      ifParams.l3a_vrf = vrf;
#else
      ifParams.l3a_vrf = 0;     // Currently VRF is always zero anyway
#endif
      memcpy(&ifParams.l3a_mac_addr, Intf->getMac().bytes(),
             sizeof(ifParams.l3a_mac_addr));
      ifParams.l3a_vid = Intf->getVlanID();
      ifParams.l3a_mtu = Intf->getMtu();
      return ifParams;
  };

  auto ifParams = intftoIfparam(intf);
  // create the interface if needed
  if (bcmIfId_ == INVALID) {
    // create a BCM l3 interface
    bool addInterface = false;
    const auto warmBootCache = hw_->getWarmBootCache();
    auto vlanMac2IntfItr =
      warmBootCache->findL3Intf(intf->getVlanID(), intf->getMac());
    if (vlanMac2IntfItr != warmBootCache->vlanAndMac2Intf_end()) {

      const auto& existingIntf = vlanMac2IntfItr->second;
      bcmIfId_ = existingIntf.l3a_intf_id;
      if (updateIntfIfNeeded(ifParams, existingIntf)) {
        // Set add interface to true, we will no issue the call to add
        // but with the above flags set this will cause the entry to be
        // updated.
        addInterface = true;
      } else {
        XLOG(DBG1) << "Interface for vlan " << intf->getVlanID() << " and mac "
                   << intf->getMac() << " already exists";
      }

    } else {
      addInterface = true;
    }
    if (addInterface) {
      if (vlanMac2IntfItr == warmBootCache->vlanAndMac2Intf_end()) {
        XLOG(DBG1) << "Adding interface for vlan : " << intf->getVlanID()
                   << " and mac: " << intf->getMac();
      }
      auto rc = opennsl_l3_intf_create(hw_->getUnit(), &ifParams);
      bcmCheckError(rc, "failed to create L3 interface ", intf->getID());
      bcmIfId_ = ifParams.l3a_intf_id;
    }
    if (vlanMac2IntfItr != warmBootCache->vlanAndMac2Intf_end()) {
      warmBootCache->programmed(vlanMac2IntfItr);
    }
    CHECK_NE(bcmIfId_, INVALID);
  }

  auto createHost = [&](const IPAddress& addr, InterfaceID intfID) {
    try {
      auto hostKey = BcmHostKey(vrf, addr, intfID);
      auto host = hw_->writableHostTable()->incRefOrCreateBcmHost(hostKey);
      auto ret = hosts_.insert(hostKey);
      CHECK(ret.second);
      SCOPE_FAIL {
        hw_->writableHostTable()->derefBcmHost(hostKey);
        hosts_.erase(ret.first);
      };
      host->programToCPU(bcmIfId_);
    } catch (const std::exception& ex) {
      XLOG(ERR) << "failed to allocate BcmHost for " << addr
                << " Error:" << folly::exceptionStr(ex);
    }
  };
  auto removeHost = [&](const IPAddress& addr, InterfaceID intfID) {
    auto hostKey = BcmHostKey(vrf, addr, intfID);
    auto iter = hosts_.find(hostKey);
    if (iter == hosts_.cend()) {
      return;
    }
    hw_->writableHostTable()->derefBcmHost(hostKey);
    hosts_.erase(iter);
  };

  if (!oldIntf) {
    // create host entries for all network IP addresses
    for (const auto& addr : intf->getAddresses()) {
      createHost(addr.first, intf->getID());
    }
  } else {
    auto oldIfParams = intftoIfparam(oldIntf);
    if (updateIntfIfNeeded(ifParams, oldIfParams)) {
      auto rc = opennsl_l3_intf_create(hw_->getUnit(), &ifParams);
      bcmCheckError(rc, "failed to update L3 interface ", intf->getID());
    }
    // address change
    auto oldIter = oldIntf->getAddresses().begin();
    auto newIter = intf->getAddresses().begin();
    while (oldIter != oldIntf->getAddresses().end()
           && newIter != intf->getAddresses().end()) {
      if (oldIter->first == newIter->first) {
        oldIter++;
        newIter++;
        continue;
      }
      if (oldIter->first < newIter->first) {
        removeHost(oldIter->first, oldIntf->getID());
        oldIter++;
      } else {
        createHost(newIter->first, intf->getID());
        newIter++;
      }
    }
    for (; oldIter != oldIntf->getAddresses().end(); oldIter++) {
      removeHost(oldIter->first, oldIntf->getID());
    }
    for (; newIter != intf->getAddresses().end(); newIter++) {
      createHost(newIter->first, intf->getID());
    }
  }
  // all new info have been programmed, store the interface configuration
  intf_ = intf;
  XLOG(DBG3) << "updated L3 interface " << bcmIfId_ << " for interface ID "
             << intf->getID();
}

BcmIntf::~BcmIntf() {
  if (bcmIfId_ == INVALID) {
    return;
  }
  // remove all BcmHost objects
  if (hw_->writableHostTable() != nullptr) {
    for (const auto& key : hosts_) {
      hw_->writableHostTable()->derefBcmHost(key);
    }
  }
  opennsl_l3_intf_t ifParams;
  opennsl_l3_intf_t_init(&ifParams);
  ifParams.l3a_intf_id = bcmIfId_;
  ifParams.l3a_flags |= OPENNSL_L3_WITH_ID;
  auto rc = opennsl_l3_intf_delete(hw_->getUnit(), &ifParams);
  bcmLogFatal(rc, hw_, "failed to delete L3 interface ", bcmIfId_);
  XLOG(DBG3) << "deleted L3 interface " << bcmIfId_;
}

folly::dynamic BcmIntf::toFollyDynamic() const {
  folly::dynamic intf = folly::dynamic::object;
  if (intf_ != nullptr) {
    intf[kVlan] = static_cast<uint16_t>(intf_->getVlanID());
    intf[kMac] = intf_->getMac().toString();
    intf[kMtu] = intf_->getMtu();
  }
  intf[kIntfId] = getBcmIfId();
  return intf;
}

BcmIntfTable::BcmIntfTable(const BcmSwitch *hw) : hw_(hw) {
}

BcmIntfTable::~BcmIntfTable() {
}

BcmIntf* BcmIntfTable::getBcmIntfIf(opennsl_if_t id) const {
  auto iter = bcmIntfs_.find(id);
  if (iter == bcmIntfs_.end()) {
    return nullptr;
  }
  return iter->second;
}

BcmIntf* BcmIntfTable::getBcmIntf(opennsl_if_t id) const {
  auto ptr = getBcmIntfIf(id);
  if (ptr == nullptr) {
    throw FbossError("Cannot find interface ", id);
  }
  return ptr;
}

BcmIntf* BcmIntfTable::getBcmIntfIf(InterfaceID id) const {
  auto iter = intfs_.find(id);
  if (iter == intfs_.end()) {
    return nullptr;
  }
  return iter->second.get();
}

BcmIntf* BcmIntfTable::getBcmIntf(InterfaceID id) const {
  auto ptr = getBcmIntfIf(id);
  if (ptr == nullptr) {
    throw FbossError("Cannot find interface ", id);
  }
  return ptr;
}

void BcmIntfTable::addIntf(const shared_ptr<Interface>& intf) {
  auto newIntf = unique_ptr<BcmIntf>(new BcmIntf(hw_));
  auto intfPtr = newIntf.get();
  auto ret = intfs_.insert(make_pair(intf->getID(), std::move(newIntf)));
  if (!ret.second) {
    throw FbossError("Adding an existing interface ", intf->getID());
  }
  intfPtr->program(intf);
  auto ret2 = bcmIntfs_.insert(make_pair(intfPtr->getBcmIfId(), intfPtr));
  CHECK_EQ(ret2.second, true);
}

void BcmIntfTable::programIntf(const shared_ptr<Interface>& intf) {
  auto intfPtr = getBcmIntf(intf->getID());
  intfPtr->program(intf);
}

void BcmIntfTable::deleteIntf(const std::shared_ptr<Interface>& intf) {
  auto iter = intfs_.find(intf->getID());
  if (iter == intfs_.end()) {
    throw FbossError("Failed to delete a non-existing interface ",
                     intf->getID());
  }
  auto bcmIfId = iter->second->getBcmIfId();
  intfs_.erase(iter);
  bcmIntfs_.erase(bcmIfId);
}

folly::dynamic BcmIntfTable::toFollyDynamic() const {
  folly::dynamic intfsJson = folly::dynamic::array;
  for (const auto& intf: intfs_) {
    intfsJson.push_back(intf.second->toFollyDynamic());
  }
  folly::dynamic intfTable = folly::dynamic::object;
  return intfTable[kIntfs] = std::move(intfsJson);
  return intfTable;
}

}} // namespace facebook::fboss
