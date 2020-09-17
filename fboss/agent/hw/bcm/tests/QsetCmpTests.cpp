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

  // Just doing a Qset cmp on qsets obtained from configured groups fails
  ASSERT_FALSE(
      qsetsEqual(getAclQset(getAsic()->getAsicType()), aclEffectiveQset));

  // Comparing via FPGroupDesiredQsetCmp succeeds when comparing qsets
  // of the same group
  ASSERT_TRUE(
      FPGroupDesiredQsetCmp(
          getUnit(),
          static_cast<bcm_field_group_t>(getAsic()->getDefaultACLGroupID()),
          getAclQset(getAsic()->getAsicType()))
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
