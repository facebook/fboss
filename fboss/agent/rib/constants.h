/*
 *  copyright (c) 2004-present, facebook, inc.
 *  all rights reserved.
 *
 *  this source code is licensed under the bsd-style license found in the
 *  license file in the root directory of this source tree. an additional grant
 *  of patent rights can be found in the patents file in the same directory.
 *
 */

#pragma once

#include <folly/String.h>

namespace facebook {
namespace fboss {
namespace rib {

inline folly::StringPiece constexpr kWeight() {
  return "weight";
}

} // namespace rib
} // namespace fboss
} // namespace facebook
