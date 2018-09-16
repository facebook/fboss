/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/rib/RouteTypes.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

TEST(RoutePrefix, Initialization) {
  RoutePrefixV4 v4Prefix;
  RoutePrefixV6 v6Prefix;
}
