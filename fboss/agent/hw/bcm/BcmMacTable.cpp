/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmMacTable.h"

#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmError.h"

namespace facebook::fboss {

void BcmMacTable::processMacAdded(const MacEntry* addedEntry, VlanID vlan) {
  CHECK(addedEntry);
  programMacEntry(addedEntry, vlan);
}

void BcmMacTable::processMacRemoved(const MacEntry* removedEntry, VlanID vlan) {
  CHECK(removedEntry);
  unprogramMacEntry(removedEntry, vlan);
}

void BcmMacTable::processMacChanged(
    const MacEntry* oldEntry,
    const MacEntry* newEntry,
    VlanID vlan) {
  CHECK(oldEntry);
  CHECK(newEntry);

  /*
   * BCN SDK alllows REPLACE with ADD API, and hence it is safe to program the
   * new MAC+vlan directly.
   */
  programMacEntry(newEntry, vlan);
}

void BcmMacTable::programMacEntry(const MacEntry* macEntry, VlanID vlan) {
  /*
   * If the MAC+vlan is not in the L2 table, this will add a new entry.
   * If the MAC+vlan is already in L2 table, this will overwrite that entry.
   * The latter should be common case, typical sequence would be:
   *  - ASIC seens unknown srcMAC + Vlan (L2 table miss).
   *  - It adds srcMAC + Vlan to L2 table as PENDING entry.
   *  - SDK generates a callback to agent.
   *  - In response, agent would 'validate' (move l2 entry from PENDING to
   *    non-PENDING) state by overwriting as below.
   *
   * Subsequently, LookupClassUpdater may decide to associate/disassociate
   * classID with this L2 entry. Again, it overwrites the previous entry
   * with/without classID.
   */
  bcm_l2_addr_t l2Addr;
  bcm_mac_t macBytes;
  macToBcm(macEntry->getMac(), &macBytes);
  auto classID = macEntry->getClassID().has_value()
      ? static_cast<int>(macEntry->getClassID().value())
      : 0;

  bcm_l2_addr_t_init(&l2Addr, macBytes, vlan);
  l2Addr.flags = BCM_L2_NATIVE;
  if (macEntry->getType() == MacEntryType::STATIC_ENTRY) {
    l2Addr.flags |= BCM_L2_STATIC;
  }
  l2Addr.group = classID;

  if (macEntry->getPort().isPhysicalPort()) {
    l2Addr.port = macEntry->getPort().phyPortID();
  } else if (macEntry->getPort().isAggregatePort()) {
    l2Addr.port = macEntry->getPort().aggPortID();
    l2Addr.flags |= BCM_L2_TRUNK_MEMBER;
  } else {
    CHECK(false) << "Invalid port type, only phys and aggr ports are supported";
  }

  auto rv = bcm_l2_addr_add(hw_->getUnit(), &l2Addr);
  bcmCheckError(
      rv,
      "Unable to add mac entry: ",
      macEntry->getMac().toString(),
      " vlanID: ",
      vlan,
      " ",
      macEntry->getPort().str());
}

void BcmMacTable::unprogramMacEntry(const MacEntry* macEntry, VlanID vlan) {
  bcm_mac_t macBytes;
  macToBcm(macEntry->getMac(), &macBytes);

  /*
   * In our implementation, typical sequence would be:
   *  - L2 entry is aged out by SDK - this removes the entry from L2 table.
   *  - SDK generates a callback to agent.
   *  - In response, agent would remove the L2 entry from its local
   *    SwitchState and attempt to remove the L2 entry from ASIC L2 table
   *    using below.
   *  - Thus, we would get bcm_E_NOT_FOUND in common case.
   *
   *  MAC Move case:
   *  - When a MAC Moves from Port P1 to port P2, wedge_agent gets two
   *    callbacks viz. DELETE for the MAC on P1 followed by ADD for the same
   *    MAC on P2.
   *  - We don't want handling of DELETE for the MAC on P1 to inadvertently
   *    DELETE the MAC that has already moved to P2. Thus, we explicitly delete
   *    the MAC on P1 (if it exists) using bcm_l2_addr_delete_by_mac_port and
   *    passing port P1 has the portID.
   *  - There is no equivalent bcm_l2_addr_delete_by_mac_trunk API, thus
   *    unconditionally delete the MAC on the trunk for now. CS9347300
   *    requests Broadcom to provide similar API.
   *
   *    bcm_l2_addr_delete_by_mac_port does not take vlan as argument, so same
   *    MAC, if it is learned on multiple VLANs on the same port, will get
   *    deleted from all the vlans. This should be rare in practice and even if
   *    it happens, it would work as the MAC would get relearned if there is
   *    ongoing traffic.
   */
  int rv = BCM_E_NONE;
  switch (macEntry->getPort().type()) {
    case PortDescriptor::PortType::PHYSICAL:
      rv = bcm_l2_addr_delete_by_mac_port(
          hw_->getUnit(),
          macBytes,
          static_cast<bcm_module_t>(0),
          macEntry->getPort().phyPortID(),
          0);
      break;
    case PortDescriptor::PortType::AGGREGATE:
      // No bcm_l2_addr_delete_by_mac_trunk API, CS9347300 requests the same.
      rv = bcm_l2_addr_delete(hw_->getUnit(), macBytes, vlan);
      break;
  }

  if (!(rv == BCM_E_NONE || rv == BCM_E_NOT_FOUND)) {
    bcmCheckError(
        rv,
        "Unable to remove mac entry: ",
        macEntry->getMac().toString(),
        " vlanID: ",
        vlan,
        " ",
        macEntry->getPort().str());
  }
}

} // namespace facebook::fboss
