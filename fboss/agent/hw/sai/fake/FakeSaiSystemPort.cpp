/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/fake/FakeSaiSystemPort.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include "fboss/agent/hw/sai/api/AddressUtil.h"

#include <folly/logging/xlog.h>
#include <optional>

using facebook::fboss::FakeSai;
using facebook::fboss::FakeSystemPort;

namespace facebook::fboss {
void populate_system_port_api(sai_system_port_api_t** /*system_port_api*/) {
  // TODO
}
} // namespace facebook::fboss
