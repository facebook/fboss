/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sim/SimHandler.h"

#include "fboss/agent/hw/sim/SimSwitch.h"

using folly::ByteRange;
using folly::IOBuf;
using std::make_unique;
using folly::StringPiece;
using std::unique_ptr;

namespace facebook { namespace fboss {

SimHandler::SimHandler(SwSwitch* sw, SimSwitch* hw)
  : ThriftHandler(sw) {
}
}} // facebook::fboss
