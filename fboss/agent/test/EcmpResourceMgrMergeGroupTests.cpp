/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/BaseEcmpResourceMgrMergeGroupsTests.h"

namespace facebook::fboss {

class EcmpResourceMgrMergeGroupTest
    : public BaseEcmpResourceMgrMergeGroupsTest {
 public:
  void setupFlags() const override {
    FLAGS_enable_ecmp_resource_manager = true;
    FLAGS_ecmp_resource_percentage = 35;
  }
};

// Base class add 5 groups, which is within in the
// Ecmp resource mgr limit.
TEST_F(EcmpResourceMgrMergeGroupTest, init) {}
} // namespace facebook::fboss
