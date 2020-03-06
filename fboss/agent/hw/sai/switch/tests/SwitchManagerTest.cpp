/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/AdapterKeySerializers.h"
#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <gtest/gtest.h>

#include <vector>

using namespace facebook::fboss;

class SwitchManagerTest : public ManagerTestBase {};

TEST_F(SwitchManagerTest, serDeser) {
  auto saiSwitch = static_cast<SaiSwitch*>(saiPlatform->getHwSwitch());
  auto adapterKeys = saiSwitch->toFollyDynamic();
  std::vector<typename SaiSwitchTraits::AdapterKey> switchKeys;
  for (auto obj : adapterKeys[kAdapterKeys][saiObjectTypeToString(
           SaiSwitchTraits::ObjectType)]) {
    switchKeys.push_back(fromFollyDynamic<SaiSwitchTraits>(obj));
  }
  EXPECT_EQ(
      std::vector<SaiSwitchTraits::AdapterKey>{saiSwitch->getSwitchId()},
      switchKeys);
}
