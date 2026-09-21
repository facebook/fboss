/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmExactMatchUtils.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"

using facebook::fboss::bcmCheckError;

namespace {
// From start bit i.e (128-prefixLength) to end bit 127 matches the
// number of bits MSB of destination Ip.
constexpr int kDefaultExactMatchDestIpHintEndBit = 127;
} // namespace

namespace facebook::fboss::utility {

int getEMGroupID(int gid) {
  return BCM_FIELD_EM_ID_BASE + gid;
}

void initEMTable(
    int unit,
    bcm_field_group_t gid,
    bcm_field_hintid_t& hintId,
    int dstIpPrefixLength) {
  bcm_field_group_config_t groupConfig;
  bcm_field_hint_t hint;

  /* Set hash multi move depth for EM */
  auto rv =
      bcm_switch_control_set(unit, bcmSwitchHashMultiMoveDepthExactMatch, 0x3);
  bcmCheckError(rv, "failed to set hash multi move for EM");

  /* Creating hint id to associate with EM Group */
  rv = bcm_field_hints_create(unit, &hintId);
  bcmCheckError(rv, "init EM Table:bcm_field_hints_create failed");
  XLOG(DBG1) << "init EM Table:Created HintId: " << hintId;

  /* configuring hint type, number of bits and the qualifier */
#if (defined(IS_OPENNSA))
  memset(&hint, 0, sizeof(bcm_field_hint_t));
#else
  bcm_field_hint_t_init(&hint);
#endif
  hint.hint_type = bcmFieldHintTypeExtraction;
  hint.qual = bcmFieldQualifyDstIp6;
  hint.start_bit = getEmDstIpHintStartBit(dstIpPrefixLength);
  hint.end_bit = kDefaultExactMatchDestIpHintEndBit;

  /* Associating the above configured hints to hint id */
  rv = bcm_field_hints_add(unit, hintId, &hint);
  bcmCheckError(rv, "init EM Table:bcm_field_hints_add failed");

  /* IFP Group Configuration and Creation */
  bcm_field_group_config_t_init(&groupConfig);
  groupConfig.group = getEMGroupID(gid);
  groupConfig.flags |= BCM_FIELD_GROUP_CREATE_WITH_MODE;
  groupConfig.flags |= BCM_FIELD_GROUP_CREATE_WITH_ID;
  /* Associating hints wth IFP Group */
  groupConfig.hintid = hintId;

  groupConfig.mode = bcmFieldGroupModeAuto;
  BCM_FIELD_QSET_ADD(groupConfig.qset, bcmFieldQualifyStageIngressExactMatch);
  BCM_FIELD_QSET_ADD(groupConfig.qset, bcmFieldQualifyDstIp6);
  BCM_FIELD_QSET_ADD(groupConfig.qset, bcmFieldQualifySrcPort);
  BCM_FIELD_ASET_ADD(groupConfig.aset, bcmFieldActionL3Switch);
  BCM_FIELD_ASET_ADD(groupConfig.aset, bcmFieldActionStatGroup);

  rv = bcm_field_group_config_create(unit, &groupConfig);
  bcmCheckError(rv, "init EM Table:bcm_field_group_config_create failed");
}

void reInitEMTable(
    int unit,
    bcm_field_group_t gid,
    bcm_field_hintid_t& hintId,
    int dstIpPrefixLength) {
  XLOG(DBG1) << "Delete EM group: " << gid;

  int entryCount = 0;
  auto rv = bcm_field_entry_multi_get(
      unit, getEMGroupID(gid), 0, nullptr, &entryCount);
  bcmCheckError(rv, "Could not get number of entries in EM table: ", gid);
  if (entryCount) {
    throw FbossError("EM table has entries. Reinit of EM table not allowed");
  }
  // TODO -store all old entries
  // Delete all EM entries, EM group
  clearFPGroup(unit, getEMGroupID(gid));

  // Delete hint id
  if (hintId) {
    rv = bcm_field_hints_destroy(unit, hintId);
    bcmCheckError(rv, "Failed to destroy hint id: ", hintId);
  }

  // init again with new hint start bit
  initEMTable(unit, gid, hintId, getEmDstIpHintStartBit(dstIpPrefixLength));
  // TODO -recreate old entries
}

bool validateDstIpHint(
    int unit,
    bcm_field_hintid_t hintId,
    int hintStartBit,
    int hintEndBit) {
  bcm_field_hint_t hint;
#if (defined(IS_OPENNSA))
  memset(&hint, 0, sizeof(bcm_field_hint_t));
#else
  bcm_field_hint_t_init(&hint);
#endif
  hint.hint_type = bcmFieldHintTypeExtraction;
  hint.qual = bcmFieldQualifyDstIp6;
  auto rv = bcm_field_hints_get(unit, hintId, &hint);
  bcmCheckError(rv, "Unable to get hints for hint: ", hintId);
  return (hint.start_bit == hintStartBit) && (hint.end_bit == hintEndBit);
}

int getEmDstIpHintStartBit(int dstIpPrefixLength) {
  return kDefaultExactMatchDestIpHintEndBit - dstIpPrefixLength + 1;
}

int getEmDstIpHintEndBit() {
  return kDefaultExactMatchDestIpHintEndBit;
}

} // namespace facebook::fboss::utility
