/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"

#include <string>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class VirtualRouterManagerTest : public ManagerTestBase {};

TEST_F(VirtualRouterManagerTest, defaultVirtualRouterTest) {
  SaiVirtualRouterHandle* virtualRouterHandle =
      saiManagerTable->virtualRouterManager().getVirtualRouterHandle(
          RouterID(0));
  EXPECT_TRUE(virtualRouterHandle);
  EXPECT_EQ(virtualRouterHandle->virtualRouter->adapterKey(), 0);
}
