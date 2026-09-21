/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/fsdb/oper/instantiations/templates/FsdbThriftStructOperInstantiations.h"

namespace facebook::fboss::thrift_cow {

// Explicit instantiation for ThriftStructFields<AgentData>
template struct ThriftStructFields<
    fsdb::AgentData,
    ThriftStructNode<fsdb::AgentData>>;

} // namespace facebook::fboss::thrift_cow
