/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <folly/Memory.h>
#include "fboss/agent/Main.h"
#include "fboss/agent/platforms/wedge/WedgePlatform.h"

using namespace facebook::fboss;
using folly::make_unique;
using std::unique_ptr;

namespace facebook { namespace fboss {

unique_ptr<Platform> initWedgePlatform() {
  return make_unique<facebook::fboss::WedgePlatform>();
}

}}

int main(int argc, char* argv[]) {
  return facebook::fboss::fbossMain(argc, argv, initWedgePlatform);
}
