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

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"

extern "C" {
#include <bcm/field.h>

#if (!defined(BCM_VER_MAJOR))
// TODO (skhare) Find OpenNSA method for this
#define sal_memset memset
#else
#include <sal/core/libc.h>
#endif
}

using namespace facebook::fboss::utility;

namespace facebook::fboss {

TEST_F(BcmTest, fpGroupExists) {
  // Create the BcmSwitch
  constexpr bcm_field_group_t kGid = 1;
  constexpr int32_t kPri = 1;
  ASSERT_FALSE(fpGroupExists(getUnit(), kGid));

  bcm_field_qset_t qset;
  BCM_FIELD_QSET_INIT(qset);
  // TH does not allow creating groups with empty QSETs
  BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstIp6);
  createFPGroup(getUnit(), qset, kGid, kPri);
  ASSERT_TRUE(fpGroupExists(getUnit(), kGid));
}

TEST_F(BcmTest, fpOnlyDesiredGrpsConfigured) {
  std::vector<bcm_field_group_t> expectedGids = {static_cast<bcm_field_group_t>(
      getPlatform()->getAsic()->getDefaultACLGroupID())};
  std::sort(expectedGids.begin(), expectedGids.end());
  auto configuredGids = fpGroupsConfigured(getUnit());
  std::sort(configuredGids.begin(), configuredGids.end());
  ASSERT_EQ(expectedGids, configuredGids);
}

TEST_F(BcmTest, fpDesiredGrpsExist) {
  // Create the BcmSwitch
  ASSERT_TRUE(fpGroupExists(
      getUnit(), getPlatform()->getAsic()->getDefaultACLGroupID()));
}

TEST_F(BcmTest, fpNoMissingOrQsetChangedGrpsPostInit) {
  ASSERT_FALSE(getHwSwitch()->haveMissingOrQSetChangedFPGroups());
}

} // namespace facebook::fboss
