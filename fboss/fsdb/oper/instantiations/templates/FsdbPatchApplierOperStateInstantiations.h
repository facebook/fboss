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

#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/thrift_cow/nodes/ThriftStructNode-inl.h"
#include "fboss/thrift_cow/visitors/PatchApplier.h"
#include "fboss/thrift_cow/visitors/PathVisitor.h"

namespace facebook::fboss::thrift_cow {

extern template PatchApplyResult
PatchApplier<apache::thrift::type_class::structure>::apply<
    ThriftStructNode<fsdb::FsdbOperStateRoot>>(
    ThriftStructNode<fsdb::FsdbOperStateRoot>& node,
    PatchNode&& patch,
    const fsdb::OperProtocol& protocol);

extern template PatchApplyResult pa_detail::patchNode<
    apache::thrift::type_class::structure,
    ThriftStructNode<fsdb::FsdbOperStateRoot>>(
    ThriftStructNode<fsdb::FsdbOperStateRoot>& node,
    ByteBuffer&& buf,
    const fsdb::OperProtocol& protocol);

} // namespace facebook::fboss::thrift_cow
