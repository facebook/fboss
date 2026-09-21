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

#include <folly/IPAddress.h>

#include <set>

namespace facebook::fboss {
class HwSwitch;
namespace utility {

bool getPtpTcEnabled(const facebook::fboss::HwSwitch* hw);

} // namespace utility
} // namespace facebook::fboss
