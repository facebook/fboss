/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/wedge/WedgePlatformInit.h"
#include "fboss/agent/test/MultiNodeTest.h"

int main(int argc, char* argv[]) {
  // temporarly set pktio to false to prevent crash in shutdown path
  std::vector<const char*> newArgv(argv, argv + argc);
  newArgv.push_back("--use_pktio=false");
  newArgv.push_back("nullptr");
  argc++;
  return facebook::fboss::mulitNodeTestMain(
      argc, (char**)newArgv.data(), facebook::fboss::initWedgePlatform);
}
