/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/dataplane_tests/HwLoadBalancerTests.h"

namespace facebook::fboss {

template class HwLoadBalancerTest<utility::HwIpV4EcmpDataPlaneTestUtil>;
template class HwLoadBalancerTest<utility::HwIpV6EcmpDataPlaneTestUtil>;

} // namespace facebook::fboss
