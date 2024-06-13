// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/bspmapping/Parser.h"

#include <folly/String.h>
#include "fboss/lib/if/gen-cpp2/fboss_common_types.h"
#include "fboss/lib/platforms/PlatformMode.h"

namespace facebook::fboss {

std::string Parser::getNameFor(PlatformType platform) {
  std::string name = facebook::fboss::toString(platform);
  folly::toLowerAscii(name);
  return name;
}
} // namespace facebook::fboss
