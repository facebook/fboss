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

#include <folly/dynamic.h>
#include <folly/FBString.h>

#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook { namespace fboss {

/*
 * Utility class to handle serializing MatchActions
 */
class MatchAction {
 public:
  static const cfg::MatchAction fromFollyDynamic(const folly::dynamic& json);
  static const folly::dynamic toFollyDynamic(const cfg::MatchAction& action);
};

}} // facebook::fboss
