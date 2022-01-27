// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestPacketTrapEntry.h"
#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorFBConvertors.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

extern "C" {
#include <bcm/field.h>
}

namespace facebook::fboss {

HwTestPacketTrapEntry::HwTestPacketTrapEntry(
    HwSwitch* hwSwitch,
    const std::set<PortID>& ports)
    : hwSwitch_(hwSwitch) {
  auto unit = static_cast<facebook::fboss::BcmSwitch*>(hwSwitch_)->getUnit();
  const bcm_field_group_t gid =
      static_cast<const facebook::fboss::BcmSwitch*>(hwSwitch)
          ->getPlatform()
          ->getAsic()
          ->getDefaultACLGroupID();
  int priority = 1;
  for (auto port : ports) {
    int entry;
    auto rv = bcm_field_entry_create(unit, gid, &entry) ||
        bcm_field_qualify_SrcPort(unit, entry, 0, 0xff, port, 0xff) ||
        bcm_field_action_add(unit, entry, bcmFieldActionCopyToCpu, 0, 0) ||
        bcm_field_entry_prio_set(
                  unit, entry, utility::swPriorityToHwPriority(priority++)) ||
        bcm_field_entry_install(unit, entry);
    bcmCheckError(rv, "HwTestPacketTrapEntry creation failed");
    entries_.push_back(entry);
  }
}

HwTestPacketTrapEntry::HwTestPacketTrapEntry(
    HwSwitch* hwSwitch,
    const uint16_t l4DstPort)
    : hwSwitch_(hwSwitch) {
  auto unit = static_cast<facebook::fboss::BcmSwitch*>(hwSwitch_)->getUnit();
  const bcm_field_group_t gid =
      static_cast<const facebook::fboss::BcmSwitch*>(hwSwitch)
          ->getPlatform()
          ->getAsic()
          ->getDefaultACLGroupID();
  int priority = 1;
  int entry;
  auto rv = bcm_field_entry_create(unit, gid, &entry) ||
      bcm_field_qualify_L4DstPort(unit, entry, l4DstPort, 0xffff) ||
      bcm_field_action_add(unit, entry, bcmFieldActionCopyToCpu, 0, 0) ||
      bcm_field_entry_prio_set(
                unit, entry, utility::swPriorityToHwPriority(priority++)) ||
      bcm_field_entry_install(unit, entry);
  bcmCheckError(rv, "HwTestPacketTrapEntry creation failed");
  entries_.push_back(entry);

  XLOG(DBG2) << "Installed the copy to CPU acl in group " << (int)gid
             << "for l4DstPort: " << l4DstPort;
}

HwTestPacketTrapEntry::HwTestPacketTrapEntry(
    HwSwitch* hwSwitch,
    const std::set<folly::CIDRNetwork>& dstPrefixes)
    : hwSwitch_(hwSwitch) {
  auto unit = static_cast<facebook::fboss::BcmSwitch*>(hwSwitch_)->getUnit();
  const bcm_field_group_t gid =
      static_cast<const facebook::fboss::BcmSwitch*>(hwSwitch)
          ->getPlatform()
          ->getAsic()
          ->getDefaultACLGroupID();
  int priority = 1;
  for (const auto& dstPrefix : dstPrefixes) {
    bcm_ip6_t addr;
    bcm_ip6_t mask;
    facebook::fboss::networkToBcmIp6(dstPrefix, &addr, &mask);
    int entry;
    auto rv = bcm_field_entry_create(unit, gid, &entry) ||
        bcm_field_qualify_DstIp6(unit, entry, addr, mask) ||
        bcm_field_action_add(unit, entry, bcmFieldActionCopyToCpu, 0, 0) ||
        bcm_field_entry_prio_set(
                  unit, entry, utility::swPriorityToHwPriority(priority++)) ||
        bcm_field_entry_install(unit, entry);
    bcmCheckError(rv, "HwTestPacketTrapEntry creation failed");
    entries_.push_back(entry);
  }
}

HwTestPacketTrapEntry::~HwTestPacketTrapEntry() {
  auto unit = static_cast<facebook::fboss::BcmSwitch*>(hwSwitch_)->getUnit();
  for (auto entry : entries_) {
    bcm_field_entry_remove(unit, entry);
    bcm_field_action_delete(unit, entry, bcmFieldActionCopyToCpu, 0, 0);
    bcm_field_entry_destroy(unit, entry);
  }
}

} // namespace facebook::fboss
