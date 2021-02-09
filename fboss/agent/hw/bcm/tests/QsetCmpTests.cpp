/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/BcmTest.h"

#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"
#include "fboss/agent/platforms/tests/utils/BcmTestPlatform.h"

extern "C" {
#if (!defined(BCM_VER_MAJOR))
// TODO (skhare) Find OpenNSA method for this
#define sal_memset memset
#else
#include <bcm/field.h>
#include <sal/core/libc.h>
#endif
}

using namespace facebook::fboss::utility;

namespace facebook::fboss {

TEST_F(BcmTest, QsetCmp) {
  // Qset comparison with itself
  ASSERT_TRUE(qsetsEqual(
      getAclQset(getAsic()->getAsicType()),
      getAclQset(getAsic()->getAsicType())));
  auto aclEffectiveQset = getGroupQset(
      getUnit(),
      static_cast<bcm_field_group_t>(getAsic()->getDefaultACLGroupID()));
  ASSERT_TRUE(qsetsEqual(aclEffectiveQset, aclEffectiveQset));

  bool needsExtraQualifiers =
      needsExtraFPQsetQualifiers(getAsic()->getAsicType());
  // Just doing a Qset cmp on qsets obtained from configured groups
  // If needsExtraQualifiers == true, the config qset won't equal to HW
  // effective qset.
  // [Ref] FPGroupDesiredQsetCmp::getEffectiveDesiredQset()
  ASSERT_EQ(
      qsetsEqual(getAclQset(getAsic()->getAsicType()), aclEffectiveQset),
      !needsExtraQualifiers);

  // Comparing via FPGroupDesiredQsetCmp succeeds when comparing qsets
  // of the same group
  ASSERT_TRUE(
      FPGroupDesiredQsetCmp(
          getHwSwitch(),
          static_cast<bcm_field_group_t>(getAsic()->getDefaultACLGroupID()),
          getAclQset(getAsic()->getAsicType()))
          .hasDesiredQset());
}

TEST_F(BcmTest, BcmAddsExtraQuals) {
  bcm_field_qset_t qset;
  BCM_FIELD_QSET_INIT(qset);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcIp6);
  constexpr auto kTmpPriority = 9999;
  // Pick the group ID is smaller than default acl group since it's created by
  // default
  const auto kTmpGroup =
      static_cast<bcm_field_group_t>(getAsic()->getDefaultACLGroupID()) - 1;
  createFPGroup(
      getUnit(),
      qset,
      kTmpGroup,
      kTmpPriority,
      getPlatform()->getAsic()->isSupported(HwAsic::Feature::HSDK));

  auto effectiveQset = getGroupQset(getUnit(), kTmpGroup);
  // Check whether bcmFieldQualifyStage is added if needsExtraFPQsetQualifiers()
  // returns true
  ASSERT_EQ(
      BCM_FIELD_QSET_TEST(effectiveQset, bcmFieldQualifyStage) != 0,
      needsExtraFPQsetQualifiers(getAsic()->getAsicType()));
  ASSERT_FALSE(BCM_FIELD_QSET_TEST(qset, bcmFieldQualifyStage));
}

} // namespace facebook::fboss
