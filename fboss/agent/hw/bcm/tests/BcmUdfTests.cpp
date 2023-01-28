/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmUdfManager.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"
#include "fboss/agent/hw/bcm/tests/BcmTestUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"

#include <memory>

using namespace facebook::fboss;

namespace facebook::fboss {

class BcmUdfTest : public BcmTest {
 protected:
  std::shared_ptr<SwitchState> setupUdfConfiguration(bool addConfig) {
    auto udfConfigState = std::make_shared<UdfConfig>();
    cfg::UdfConfig udfConfig;
    if (addConfig) {
      udfConfig = utility::addUdfConfig();
    }
    udfConfigState->fromThrift(udfConfig);

    auto state = getProgrammedState();
    state->modify(&state);
    state->resetUdfConfig(udfConfigState);
    return state;
  }
};

TEST_F(BcmUdfTest, checkUdfGroupConfiguration) {
  auto setupUdfConfig = [=]() { applyNewState(setupUdfConfiguration(true)); };
  auto verifyUdfConfig = [=]() {
    const int udfGroupId =
        getHwSwitch()->getUdfMgr()->getBcmUdfGroupId(utility::kUdfGroupName);
    /* get udf info */
    bcm_udf_t udfInfo;
    bcm_udf_t_init(&udfInfo);
    auto rv = bcm_udf_get(getHwSwitch()->getUnit(), udfGroupId, &udfInfo);
    bcmCheckError(rv, "Unable to get udfInfo for udfGroupId: ", udfGroupId);
    ASSERT_EQ(udfInfo.layer, bcmUdfLayerL4OuterHeader);
    ASSERT_EQ(udfInfo.start, utility::kUdfStartOffsetInBytes * 8);
    ASSERT_EQ(udfInfo.width, utility::kUdfFieldSizeInBytes * 8);
  };

  verifyAcrossWarmBoots(setupUdfConfig, verifyUdfConfig);
};
} // namespace facebook::fboss
