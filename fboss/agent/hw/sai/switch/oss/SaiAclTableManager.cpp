// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"

namespace facebook::fboss {
sai_uint32_t SaiAclTableManager::swPriorityToSaiPriority(int priority) const {
  /*
   * TODO(skhare)
   * When adding HwAclPriorityTests, add a test to verify that SAI
   * implementation treats larger value of priority as higher priority.
   * SwitchState: smaller ACL ID means higher priority.
   * SAI spec: does not define?
   * But larger priority means higher priority is documented here:
   * https://github.com/opencomputeproject/SAI/blob/master/doc/SAI-Proposal-ACL-1.md
   */
  sai_uint32_t saiPriority = aclEntryMaximumPriority_ - priority;

  if (saiPriority < aclEntryMinimumPriority_ ||
      saiPriority > aclEntryMaximumPriority_) {
    throw FbossError(
        "Acl Entry priority out of range. Supported: [",
        aclEntryMinimumPriority_,
        ", ",
        aclEntryMaximumPriority_,
        "], specified: ",
        saiPriority);
  }

  return saiPriority;
}

} // namespace facebook::fboss
