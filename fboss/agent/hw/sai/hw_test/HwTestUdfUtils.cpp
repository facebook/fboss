/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiUdfManager.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"

#include <gtest/gtest.h>

namespace facebook::fboss::utility {

void validateUdfConfig(
    const HwSwitch* hw,
    const std::string& udfGroupName,
    const std::string& udfPacketMatcherName) {
  const auto saiSwitch = static_cast<const SaiSwitch*>(hw);
  const facebook::fboss::SaiUdfManager& udfManager =
      saiSwitch->managerTable()->udfManager();
  const auto udfGroupIter = udfManager.getUdfGroupHandles().find(udfGroupName);
  EXPECT_TRUE(udfGroupIter != udfManager.getUdfGroupHandles().cend());
  if (udfGroupIter != udfManager.getUdfGroupHandles().cend()) {
    // Verify UdfGroup attributes
    auto saiUdfGroup = udfGroupIter->second->udfGroup;
    const auto& udfApi = SaiApiTable::getInstance()->udfApi();
    EXPECT_EQ(
        udfApi.getAttribute(
            saiUdfGroup->adapterKey(), SaiUdfGroupTraits::Attributes::Type{}),
        SAI_UDF_GROUP_TYPE_HASH);
    EXPECT_EQ(
        udfApi.getAttribute(
            saiUdfGroup->adapterKey(), SaiUdfGroupTraits::Attributes::Length{}),
        utility::kUdfFieldSizeInBytes);

    // Verify Udf attributes
    auto saiUdf = udfGroupIter->second->udfs[udfPacketMatcherName]->udf;
    EXPECT_EQ(
        udfApi.getAttribute(
            saiUdf->adapterKey(), SaiUdfTraits::Attributes::Offset{}),
        utility::kUdfStartOffsetInBytes);
    EXPECT_EQ(
        udfApi.getAttribute(
            saiUdf->adapterKey(), SaiUdfTraits::Attributes::Base{}),
        SAI_UDF_BASE_L4);
  }
}

} // namespace facebook::fboss::utility
