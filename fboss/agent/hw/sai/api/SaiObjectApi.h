/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/hw/sai/api/SaiApiError.h"

extern "C" {
#include <sai.h>
}

/*
 * This library provide a wrapper around the facilities in sai_object.h for
 * bulk object and attribute traversal in SAI. These are primarily for use
 * during warm boot, but are not necessarily restricted to that use case.
 */

namespace facebook {
namespace fboss {

template <typename SaiObjectTraits>
uint32_t getObjectCount(sai_object_id_t switch_id) {
  uint32_t count;
  sai_status_t status =
      sai_get_object_count(switch_id, SaiObjectTraits::ObjectType, &count);
  saiCheckError(status, "Failed to get object count");
  return count;
}

} // namespace fboss
} // namespace facebook
