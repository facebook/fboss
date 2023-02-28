/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/PortPgConfig.h"
#include <folly/Conv.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <sstream>
#include "fboss/agent/state/BufferPoolConfig.h"
#include "fboss/agent/state/NodeBase-defs.h"

namespace facebook::fboss {

template class ThriftStructNode<PortPgConfig, state::PortPgFields>;

} // namespace facebook::fboss
