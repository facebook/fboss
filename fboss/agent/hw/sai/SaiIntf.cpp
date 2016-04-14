/*
 * Copyright (c) 2004-present, Facebook, Inc.
 * Copyright (c) 2016, Cavium, Inc.
 * All rights reserved.
 * 
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 * 
 */
#include <folly/IPAddress.h>

#include "SaiSwitch.h"
#include "SaiIntf.h"
#include "SaiVrf.h"
#include "SaiVrfTable.h"
#include "SaiError.h"
#include "SaiHostTable.h"
#include "SaiHost.h"
#include "SaiStation.h"

using std::shared_ptr;
using folly::IPAddress;

namespace facebook { namespace fboss {

SaiIntf::SaiIntf(const SaiSwitch *hw)
  : hw_(hw) {
  VLOG(4) << "Entering " << __FUNCTION__;

  saiRouterInterfaceApi_ = hw->GetSaiRouterIntfApi();
}

SaiIntf::~SaiIntf() {
  VLOG(4) << "Entering " << __FUNCTION__;

  if (saiIfId_ != SAI_NULL_OBJECT_ID) {

    auto iter = hosts_.begin();
    while (iter != hosts_.end()) {
      removeHost(*iter);

      iter = hosts_.begin();
    }

    saiRouterInterfaceApi_->remove_router_interface(saiIfId_);
  }

  hw_->WritableVrfTable()->DerefSaiVrf(intf_->getRouterID());
}

void SaiIntf::Program(const shared_ptr<Interface> &intf) {
  VLOG(4) << "Entering " << __FUNCTION__;
  sai_status_t saiRetVal = SAI_STATUS_FAILURE;

  const auto &oldIntf = intf_;

  if (oldIntf != nullptr) {
    // vrf and vid cannot be changed by this method.
    if (oldIntf->getRouterID() != intf->getRouterID()
        || oldIntf->getVlanID() != intf->getVlanID()) {

      throw SaiError("Interface router ID ( ",
                       static_cast<uint32_t>(oldIntf->getRouterID()), " vs ",
                       static_cast<uint32_t>(intf->getRouterID()), " ) or ",
                       "VLAN ID (", static_cast<uint16_t>(oldIntf->getVlanID()),
                       " vs ", static_cast<uint32_t>(intf->getVlanID()), 
                       " ) is changed during programming"); 
    }
  } else {
    auto vrf = hw_->WritableVrfTable()->IncRefOrCreateSaiVrf(intf->getRouterID());

    try {
      vrf->Program();
    } catch (const FbossError &e) {
      hw_->WritableVrfTable()->DerefSaiVrf(intf->getRouterID());
      // let handle this in the caller.
      throw;
    }

    uint8_t attr_count = 0;
  
    sai_attribute_t attr;
    sai_attribute_t attr_list[SAI_ROUTER_IF_ATTR_COUNT] = {0};
   
    //Interface type
    attr_list[0].id = SAI_ROUTER_INTERFACE_ATTR_TYPE;
    attr_list[0].value.u32 = SAI_ROUTER_INTERFACE_TYPE_VLAN;
    attr_count++;

    //VlanID
    attr_list[1].id = SAI_ROUTER_INTERFACE_ATTR_VLAN_ID;
    attr_list[1].value.u32 = intf->getVlanID();
    attr_count++;

    //Router ID
    attr_list[2].id = SAI_ROUTER_INTERFACE_ATTR_VIRTUAL_ROUTER_ID;
    attr_list[2].value.oid = hw_->WritableVrfTable()->GetSaiVrfId(intf->getRouterID());
    attr_count++;

    //Admin V4 state
    attr_list[3].id = SAI_ROUTER_INTERFACE_ATTR_ADMIN_V4_STATE;
    attr_list[3].value.u32 = 1;
    attr_count++;

    //Admin V6 state
    attr_list[4].id = SAI_ROUTER_INTERFACE_ATTR_ADMIN_V6_STATE;
    attr_list[4].value.u32 = 1;
    attr_count++;

    //MAC
    attr_list[5].id = SAI_ROUTER_INTERFACE_ATTR_SRC_MAC_ADDRESS;
    memcpy(&attr_list[5].value.mac, intf->getMac().bytes(), sizeof(attr_list[5].value.mac));
    attr_count++;

    sai_object_id_t rif_id = 0;
    sai_status_t saiRetVal = saiRouterInterfaceApi_->create_router_interface(&rif_id, attr_count, attr_list);

    if (saiRetVal != SAI_STATUS_SUCCESS) {
      throw SaiError("SAI create_router_interface() failed. Error: ", saiRetVal);
    }

    saiIfId_ = rif_id;
  }

  if ((oldIntf != nullptr) && (oldIntf->getMac() != intf->getMac())) {
    //MAC
    sai_attribute_t attr {0};
    attr.id = SAI_ROUTER_INTERFACE_ATTR_SRC_MAC_ADDRESS;
    memcpy(&attr.value.mac, intf->getMac().bytes(), sizeof(attr.value.mac));
    saiRetVal = saiRouterInterfaceApi_->set_router_interface_attribute(saiIfId_, &attr);
    if (saiRetVal != SAI_STATUS_SUCCESS) {
      throw SaiError("set_router_interface_attribute() failed while setting MAC: ",
                     intf->getMac(), " retVal ", saiRetVal);
    }
  }
/*  
  //MTU
  attr.id = SAI_ROUTER_INTERFACE_ATTR_MTU;
  //TODO verify what MTU should be set here
  attr.value.u32 = 1500; //ifParams.l3a_mtu = 1500;
  saiRetVal = saiRouterInterfaceApi_->set_router_interface_attribute(rif_id, &attr);
  if (saiRetVal != SAI_STATUS_SUCCESS) {
    throw SaiError("SAI create_router_interface() failed : retVal ", saiRetVal);
  }
 */

  // Need this temp variable since some strange
  // compilation errors occur when try to call 
  // intf_->getMac() from lambda functions below.
  folly::MacAddress oldMac;

  if (oldIntf != nullptr) {
      oldMac = oldIntf->getMac();
  }

  auto createHost = [&](const IPAddress& addr) {
    auto host = hw_->WritableHostTable()->createOrUpdateSaiHost(intf->getID(),
                                                                addr,
                                                                intf->getMac());

    try {
      host->Program(SAI_PACKET_ACTION_TRAP, intf->getMac());
    } catch (const FbossError &e) {
      hw_->WritableHostTable()->removeSaiHost(intf->getID(), addr);
      // let handle this in the caller.
      throw;
    }

    auto ret = hosts_.insert(addr);
    CHECK(ret.second);
    SCOPE_FAIL {
      hw_->WritableHostTable()->removeSaiHost(intf->getID(), addr);
      hosts_.erase(ret.first);
    };
  };

  auto ifsAreEqual = [&](const IPAddress& oldAddr, const IPAddress& newAddr) {
      return ((oldAddr == newAddr) &&
              (intf_->getID() == intf->getID()) && 
              (oldMac == intf->getMac()));
  };


  if (!oldIntf) {
    // create host entries for all network IP addresses
    for (const auto& addr : intf->getAddresses()) {
      createHost(addr.first);
    }
  } else {
    // address change
    auto oldIter = oldIntf->getAddresses().begin();
    auto newIter = intf->getAddresses().begin();

    while ((oldIter != oldIntf->getAddresses().end()) &&
           (newIter != intf->getAddresses().end())) {

      // If such an address has been removed or changed
      // then need to remove the old host as well
      if ((oldIter != oldIntf->getAddresses().end()) && 
          ((newIter == intf->getAddresses().end()) ||
          !ifsAreEqual(oldIter->first, newIter->first))) {

        removeHost(oldIter->first);
      }

      // If a new address has been added or the old one changed
      // then need to add a new host
      if ((newIter != intf->getAddresses().end()) && 
          ((oldIter == oldIntf->getAddresses().end()) ||
          !ifsAreEqual(oldIter->first, newIter->first))) {

        createHost(newIter->first);
      }

      ++oldIter;
      ++newIter;
    }
  }

  intf_ = intf;
}

void SaiIntf::removeHost(const folly::IPAddress& addr) {
  auto iter = hosts_.find(addr);
  if (iter == hosts_.end()) {
    return;
  }

  hw_->WritableHostTable()->removeSaiHost(intf_->getID(), addr);
  hosts_.erase(iter);
}

}} // facebook::fboss
