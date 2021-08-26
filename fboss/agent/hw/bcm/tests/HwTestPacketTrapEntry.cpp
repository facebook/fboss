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
    const std::set<PortID>& ports) {
  unit_ = static_cast<facebook::fboss::BcmSwitch*>(hwSwitch)->getUnit();
  const bcm_field_group_t gid =
      static_cast<const facebook::fboss::BcmSwitch*>(hwSwitch)
          ->getPlatform()
          ->getAsic()
          ->getDefaultACLGroupID();
  int priority = 1;
  for (auto port : ports) {
    int entry;
    auto rv = bcm_field_entry_create(unit_, gid, &entry) ||
        bcm_field_qualify_SrcPort(unit_, entry, 0, 0xff, port, 0xff) ||
        bcm_field_action_add(unit_, entry, bcmFieldActionCopyToCpu, 0, 0) ||
        bcm_field_entry_prio_set(
                  unit_, entry, utility::swPriorityToHwPriority(priority++)) ||
        bcm_field_entry_install(unit_, entry);
    bcmCheckError(rv, "HwTestPacketTrapEntry creation failed");
    entries_.push_back(entry);
  }
}

HwTestPacketTrapEntry::HwTestPacketTrapEntry(
    HwSwitch* hwSwitch,
    const std::set<folly::CIDRNetwork>& dstPrefixes) {
  unit_ = static_cast<facebook::fboss::BcmSwitch*>(hwSwitch)->getUnit();
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
    auto rv = bcm_field_entry_create(unit_, gid, &entry) ||
        bcm_field_qualify_DstIp6(unit_, entry, addr, mask) ||
        bcm_field_action_add(unit_, entry, bcmFieldActionCopyToCpu, 0, 0) ||
        bcm_field_entry_prio_set(
                  unit_, entry, utility::swPriorityToHwPriority(priority++)) ||
        bcm_field_entry_install(unit_, entry);
    bcmCheckError(rv, "HwTestPacketTrapEntry creation failed");
    entries_.push_back(entry);
  }
}

HwTestPacketTrapEntry::~HwTestPacketTrapEntry() {
  for (auto entry : entries_) {
    bcm_field_entry_remove(unit_, entry);
    bcm_field_action_delete(unit_, entry, bcmFieldActionCopyToCpu, 0, 0);
    bcm_field_entry_destroy(unit_, entry);
  }
}

} // namespace facebook::fboss
