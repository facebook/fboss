// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"

namespace facebook::fboss {
sai_uint32_t SaiAclTableManager::swPriorityToSaiPriority(int priority) const {
  /*
   * TODO(skhare)
   * When adding HwAclPriorityTests, add a test to verify that SAI
   * implementation treats larger value of priority as higher priority.
   * SwitchState: smaller ACL ID means higher priority.
   * Tajo SAI: larger priority means higher priority.
   * SAI spec: does not define?
   * But larger priority means higher priority is documented here:
   * https://github.com/opencomputeproject/SAI/blob/master/doc/SAI-Proposal-ACL-1.md
   */
  sai_uint32_t saiPriority;
#if defined(TAJO_SDK_VERSION_1_42_1) || defined(TAJO_SDK_VERSION_1_42_8)
  /*
   * For pre SDK1.6x based code for TAJO, maintain status quo. This is
   * to avoid a disruption during warm boot due to priority change of
   * ACL entries. This code needs to be removed once SDK1.6x is fully
   * deployed to fleet. SDK1.6x is planned to be deployed with a COLD
   * boot and hence the ACL entry priority change during the upgrade is
   * acceptable.
   *
   * TODO: Remove this block once TAJO SDK1.6x is deployed to fleet.
   */
  saiPriority = aclEntryMaximumPriority_ - priority;
#else
  /*
   * In TAJO, HW does not support range specified by the caller. Here
   * we need to map the priority provided by the caller to the supported
   * range in HW -> [aclEntryMinimumPriority_, aclEntryMaximumPriority_].
   * The upper half will be made available for CPU ACLs with higher priority
   * and the lower half will be available for dataplane ACLs.
   *
   * Example:
   * Assuming HW supported priority range as [0, N] and 100K as
   * AclTable::kDataplaneAclMaxPriority.
   *
   * CPU ACL entries have priority < 100K, highest priority CPU ACL entry
   * has priority 0, next 1, 2 etc. Translation of input priorities to
   * saiPriorities as below:
   *
   *    priority  |    saiPriority
   *    --------------------------
   *        0     |         N
   *        1     |        N-1
   *        2     |        N-2
   *             ...
   *      N/2-1   |       N/2+1
   *
   * Dataplane ACL entries have priority >= 100K, highest priority dataplane
   * ACL entry has priority 100K, next 100K+1, 100K+2 etc. Translation of
   * input priorities to saiPriorities as below:
   *
   *    priority  |    saiPriority
   *    --------------------------
   *      100K    |       N/2
   *     100K+1   |      N/2-1
   *     100K+2   |      N/2-2
   *             ...
   *    100K+N/2  |        0
   */
  auto dataplaneAclMaxPriority = aclEntryMinimumPriority_ +
      (aclEntryMaximumPriority_ - aclEntryMinimumPriority_) / 2;

  if (priority < AclTable::kDataplaneAclMaxPriority) { // CPU ACLs
    CHECK_LT(priority, aclEntryMaximumPriority_);
    saiPriority = aclEntryMaximumPriority_ - priority;
    // CPU ACLs should not overflow into dataplane ACL priorities!
    if (saiPriority <= dataplaneAclMaxPriority) {
      throw FbossError(
          "Unexpected CPU ACL entry SAI priority: ",
          saiPriority,
          ", overlapping with dataplane ACLs!");
    }
  } else { // Dataplane ACLs
    auto delta = priority - AclTable::kDataplaneAclMaxPriority;
    CHECK_LT(delta, dataplaneAclMaxPriority);
    saiPriority = dataplaneAclMaxPriority - delta;
  }
#endif

  if (saiPriority < aclEntryMinimumPriority_
#if !defined(TAJO_SDK_VERSION_1_42_1) && !defined(TAJO_SDK_VERSION_1_42_8)
      || saiPriority > aclEntryMaximumPriority_
#endif
  ) {
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
