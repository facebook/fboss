// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/tests/BcmTestPacketTrapAclEntry.h"
#include "fboss/agent/hw/bcm/BcmError.h"

DECLARE_int32(acl_gid);

namespace facebook::fboss {

BcmTestPacketTrapAclEntry::BcmTestPacketTrapAclEntry(int unit, PortID port)
    : unit_(unit) {
  const bcm_field_group_t gid = FLAGS_acl_gid;
  auto rv = bcm_field_entry_create(unit_, gid, &entry_) ||
      bcm_field_qualify_SrcPort(0, entry_, 0, 0xff, port, 0xff) ||
      bcm_field_action_add(0, entry_, bcmFieldActionCopyToCpu, 0, 0) ||
      bcm_field_entry_install(unit_, entry_);
  bcmCheckError(rv, "BcmTestPacketTrapAclEntry creation failed");
}

BcmTestPacketTrapAclEntry::~BcmTestPacketTrapAclEntry() {
  bcm_field_entry_remove(unit_, entry_);
  bcm_field_action_delete(unit_, entry_, bcmFieldActionCopyToCpu, 0, 0);
  bcm_field_entry_destroy(unit_, entry_);
}

} // namespace facebook::fboss
