/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/CmdLocalOptions.h"

#include <folly/Singleton.h>

namespace {
struct singleton_tag_type {};
} // namespace

namespace facebook::fboss {

using facebook::fboss::CmdLocalOptions;
static folly::Singleton<CmdLocalOptions, singleton_tag_type> cmdLocalOptions_{};
std::shared_ptr<CmdLocalOptions> CmdLocalOptions::getInstance() {
  return cmdLocalOptions_.try_get();
}
} // namespace facebook::fboss
