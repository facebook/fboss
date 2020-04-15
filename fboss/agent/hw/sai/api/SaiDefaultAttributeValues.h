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

#include "fboss/agent/hw/sai/api/Traits.h"
#include "fboss/agent/hw/sai/api/Types.h"

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

struct SaiObjectIdDefault {
  sai_object_id_t operator()() const {
    return SAI_NULL_OBJECT_ID;
  }
};
} // namespace facebook::fboss
