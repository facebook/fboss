/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/PortApi.h"

bool operator==(const sai_map_t& lhs, const sai_map_t& rhs) {
  return lhs.key == rhs.key && lhs.value == rhs.value;
}

bool operator!=(const sai_map_t& lhs, const sai_map_t& rhs) {
  return !(lhs == rhs);
}
