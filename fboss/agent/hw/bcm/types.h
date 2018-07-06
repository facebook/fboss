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

#include "fboss/agent/types.h"

FBOSS_STRONG_TYPE(int, BcmAclStatHandle);

namespace facebook {
namespace fboss {

using BcmAclEntryHandle = int;
using BcmAclRangeHandle = uint32_t;

} // namespace fboss
} // namespace facebook
