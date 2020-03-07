/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestMplsUtils.h"
#include "fboss/agent/hw/sai/api/MplsApi.h"

#include <folly/gen/Base.h>

namespace facebook::fboss::utility {

/*
 * Retrieve the next hop ID from the label entry
 */
int getLabelSwappedWithForTopLabel(uint32_t label) {
  // TDB
  return 1101;
}

} // namespace facebook::fboss::utility
