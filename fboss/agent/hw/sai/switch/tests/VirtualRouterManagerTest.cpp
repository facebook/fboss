/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

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
  EXPECT_EQ(virtualRouterHandle->virtualRouter->adapterKey(), uint64_t(0));

  EXPECT_TRUE(virtualRouterHandle->mplsRouterInterface);

  EXPECT_EQ(
      GET_ATTR(
          MplsRouterInterface,
          Type,
          virtualRouterHandle->mplsRouterInterface->attributes()),
      SAI_ROUTER_INTERFACE_TYPE_MPLS_ROUTER);
  EXPECT_EQ(
      GET_ATTR(
          MplsRouterInterface,
          VirtualRouterId,
          virtualRouterHandle->mplsRouterInterface->attributes()),
      virtualRouterHandle->virtualRouter->adapterKey());
}

TEST_F(VirtualRouterManagerTest, getRouterIDTest) {
  // Get the virtual router handle for RouterID(0)
  SaiVirtualRouterHandle* virtualRouterHandle =
      saiManagerTable->virtualRouterManager().getVirtualRouterHandle(
          RouterID(0));
  EXPECT_TRUE(virtualRouterHandle);

  // Get the SAI object ID (adapter key) from the handle
  auto vrId = virtualRouterHandle->virtualRouter->adapterKey();

  // Use the new getRouterID API to do a reverse lookup
  auto routerId = saiManagerTable->virtualRouterManager().getRouterID(vrId);

  // Verify we found the correct RouterID
  EXPECT_EQ(routerId, RouterID(0));
}

TEST_F(VirtualRouterManagerTest, getRouterIDNotFoundTest) {
  // Use an invalid SAI object ID that doesn't exist in the handles map
  sai_object_id_t invalidVrId = 999999;

  // The reverse lookup should throw FbossError
  EXPECT_THROW(
      saiManagerTable->virtualRouterManager().getRouterID(invalidVrId),
      FbossError);
}
