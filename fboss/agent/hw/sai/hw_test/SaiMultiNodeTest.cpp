/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiPlatformInit.h"
#include "fboss/agent/test/MultiNodeTest.h"

int main(int argc, char* argv[]) {
  return facebook::fboss::mulitNodeTestMain(
      argc, argv, facebook::fboss::initSaiPlatform);
}
