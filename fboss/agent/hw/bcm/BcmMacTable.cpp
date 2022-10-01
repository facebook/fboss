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
#include "fboss/agent/hw/switch_asics/HwAsic.h"

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

  // Check the l2 entry that's already programmed in the SDK. If that carries
  // the same group (ClassID) and all the flags we try to program, skip invoking
  // l2 addr add. On devices do not support l2 pending entry, there's no need to
  // validate l2 entries through the bcm_l2_addr_add call.
  // If this check is not in place, invoking l2_addr_add() on every l2 learn
  // event would trigger a cycle of l2 entry being learned/remove, since SDK
  // would handle the l2_addr_add() by a remove followed by an add (which in
  // turn creates another cycle of learn/age events).
  bcm_l2_addr_t programmedL2Addr;
  auto rv = bcm_l2_addr_get(hw_->getUnit(), macBytes, vlan, &programmedL2Addr);
  // Only check flags for l2 static and trunk.
  uint32_t mask = BCM_L2_STATIC | BCM_L2_TRUNK_MEMBER;
  if (BCM_SUCCESS(rv) &&
      !hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::PENDING_L2_ENTRY) &&
      (mask & programmedL2Addr.flags) == (mask & l2Addr.flags) &&
      l2Addr.group == programmedL2Addr.group &&
      l2Addr.port == programmedL2Addr.port) {
    auto macAddrStr = folly::sformat(
        "{0:02x}:{1:02x}:{2:02x}:{3:02x}:{4:02x}:{5:02x}",
        l2Addr.mac[0],
        l2Addr.mac[1],
        l2Addr.mac[2],
        l2Addr.mac[3],
        l2Addr.mac[4],
        l2Addr.mac[5]);
    XLOG(DBG2) << "Skip programming mac entry " << macAddrStr
               << " since SDK carries the same l2 entry";
    return;
  }

  rv = bcm_l2_addr_add(hw_->getUnit(), &l2Addr);
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
  // If Pending L2 entry feature is not supported, meaning there's no real hw
  // learning in the ASIC (l2 learning is implemented in the SDK layer, and
  // there's no need to validate the pending state of l2 entries). Therefore, no
  // need to remove mac entry from the SDK.
  if (!hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::PENDING_L2_ENTRY)) {
    return;
  }
  bcm_mac_t macBytes;
  macToBcm(macEntry->getMac(), &macBytes);

  int rv = BCM_E_NONE;
  bcm_l2_addr_t programmedL2Addr;
  rv = bcm_l2_addr_get(hw_->getUnit(), macBytes, vlan, &programmedL2Addr);
  if (BCM_SUCCESS(rv)) {
    XLOG(DBG2) << "get existing mac entry for " << macEntry->getMac().toString()
               << " vlan " << vlan << " mask "
               << (BCM_L2_STATIC & programmedL2Addr.flags) << " port "
               << programmedL2Addr.port;
  } else {
    XLOG(DBG2) << "failed to get existing mac entry  for "
               << macEntry->getMac().toString() << " vlan " << vlan << " rv "
               << rv;
    return;
  }

  // Removing non-static entry is triggered by SDK L2 delete callback, no need
  // to really unprogram the entry, since the L2 entry is already deleted by
  // SDK. If we call unprogram again below, in normal case, it would be a no-op.
  // In rare case, it may delete another L2 entry re-learnt in quick succession
  // and cause unexpected behavior.
  if ((BCM_L2_STATIC & programmedL2Addr.flags) == 0) {
    XLOG(DBG2) << "Skip unprogramming non-static mac entry"
               << macEntry->getMac().toString() << " vlan " << vlan << " port "
               << macEntry->getPort().str()
               << " since it is already deleted by SDK";
    return;
  }

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
  switch (macEntry->getPort().type()) {
    case PortDescriptor::PortType::PHYSICAL:
      rv = bcm_l2_addr_delete_by_mac_port(
          hw_->getUnit(),
          macBytes,
          static_cast<bcm_module_t>(0),
          macEntry->getPort().phyPortID(),
          BCM_L2_REPLACE_NO_CALLBACKS);
      break;
    case PortDescriptor::PortType::AGGREGATE:
      // No bcm_l2_addr_delete_by_mac_trunk API, CS9347300 requests the same.
      rv = bcm_l2_addr_delete(hw_->getUnit(), macBytes, vlan);
      break;
    case PortDescriptor::PortType::SYSTEM_PORT:
      XLOG(FATAL) << " Mac entries over system ports are not expected";
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
