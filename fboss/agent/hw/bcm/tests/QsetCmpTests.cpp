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
#include "fboss/agent/platforms/test_platforms/BcmTestPlatform.h"

extern "C" {
#if (!defined(BCM_VER_MAJOR))
// TODO (skhare) Find OpenNSA method for this
#define sal_memset memset
#else
#include <bcm/field.h>
#include <sal/core/libc.h>
#endif
}

DECLARE_int32(acl_gid);
DECLARE_int32(ll_mcast_gid);

using namespace facebook::fboss::utility;

namespace facebook::fboss {

TEST_F(BcmTest, QsetCmp) {
  // Qset comparison with itself
  ASSERT_TRUE(qsetsEqual(
      getAclQset(getPlatform()->getAsic()->getAsicType()),
      getAclQset(getPlatform()->getAsic()->getAsicType())));
  ASSERT_TRUE(qsetsEqual(getLLMcastQset(), getLLMcastQset()));
  auto aclEffectiveQset =
      getGroupQset(getUnit(), static_cast<bcm_field_group_t>(FLAGS_acl_gid));
  auto llMcastEffectiveQset = getGroupQset(
      getUnit(), static_cast<bcm_field_group_t>(FLAGS_ll_mcast_gid));
  ASSERT_TRUE(qsetsEqual(aclEffectiveQset, aclEffectiveQset));
  ASSERT_TRUE(qsetsEqual(llMcastEffectiveQset, llMcastEffectiveQset));

  // Qset comparison with another fails
  ASSERT_FALSE(qsetsEqual(
      getAclQset(getPlatform()->getAsic()->getAsicType()), getLLMcastQset()));

  // Just doing a Qset cmp on qsets obtained from configured groups fails
  ASSERT_FALSE(qsetsEqual(
      getAclQset(getPlatform()->getAsic()->getAsicType()), aclEffectiveQset));
  ASSERT_FALSE(qsetsEqual(getLLMcastQset(), llMcastEffectiveQset));

  // Comparing via FPGroupDesiredQsetCmp succeeds when comparing qsets
  // of the same group
  ASSERT_TRUE(FPGroupDesiredQsetCmp(
                  getUnit(),
                  static_cast<bcm_field_group_t>(FLAGS_acl_gid),
                  getAclQset(getPlatform()->getAsic()->getAsicType()))
                  .hasDesiredQset());
  ASSERT_TRUE(FPGroupDesiredQsetCmp(
                  getUnit(),
                  static_cast<bcm_field_group_t>(FLAGS_ll_mcast_gid),
                  getLLMcastQset())
                  .hasDesiredQset());
  // Comparing via FPGroupDesiredQsetCmp fails when comparing qsets of
  // different groups
  ASSERT_FALSE(FPGroupDesiredQsetCmp(
                   getUnit(),
                   static_cast<bcm_field_group_t>(FLAGS_acl_gid),
                   getLLMcastQset())
                   .hasDesiredQset());
}

TEST_F(BcmTest, BcmAddsExtraQuals) {
  bcm_field_qset_t qset;
  BCM_FIELD_QSET_INIT(qset);
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcIp6);
  constexpr auto kTmpGrpAndPri = 9999;
  createFPGroup(getUnit(), qset, kTmpGrpAndPri, kTmpGrpAndPri);
  auto effectiveQset = getGroupQset(getUnit(), kTmpGrpAndPri);
  ASSERT_TRUE(BCM_FIELD_QSET_TEST(effectiveQset, bcmFieldQualifyStage));
  ASSERT_FALSE(BCM_FIELD_QSET_TEST(qset, bcmFieldQualifyStage));
}

} // namespace facebook::fboss
