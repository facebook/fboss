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
#include "fboss/agent/platforms/wedge/Wedge100Platform.h"

using namespace facebook::fboss;
using folly::make_unique;
using std::unique_ptr;

DEFINE_string(fruid_filepath, "/dev/shm/fboss/fruid.json",
              "File for storing the fruid data");

namespace facebook { namespace fboss {

unique_ptr<WedgePlatform> createPlatform() {
  auto productInfo = folly::make_unique<WedgeProductInfo>(FLAGS_fruid_filepath);
  productInfo->initialize();
  if (productInfo->getMode() == WedgePlatformMode::WEDGE100) {
    return folly::make_unique<Wedge100Platform>(std::move(productInfo));
  }
  return folly::make_unique<WedgePlatform>(std::move(productInfo));
}

unique_ptr<Platform> initWedgePlatform() {
  auto platform = createPlatform();
  platform->init();
  return std::move(platform);
}

}}

int main(int argc, char* argv[]) {
  return facebook::fboss::fbossMain(argc, argv, initWedgePlatform);
}
