/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/fsdb/oper/instantiations/templates/FsdbPatchApplierOperStateInstantiations.h"

namespace facebook::fboss::thrift_cow {

// Explicit instantiation for PatchApplier::apply with
// ThriftStructNode<FsdbOperStateRoot>
template PatchApplyResult PatchApplier<apache::thrift::type_class::structure>::
    apply<ThriftStructNode<fsdb::FsdbOperStateRoot>>(
        ThriftStructNode<fsdb::FsdbOperStateRoot>& node,
        PatchNode&& patch,
        const fsdb::OperProtocol& protocol);

// Explicit instantiation for pa_detail::patchNode with
// ThriftStructNode<FsdbOperStateRoot>
template PatchApplyResult pa_detail::patchNode<
    apache::thrift::type_class::structure,
    ThriftStructNode<fsdb::FsdbOperStateRoot>>(
    ThriftStructNode<fsdb::FsdbOperStateRoot>& node,
    ByteBuffer&& buf,
    const fsdb::OperProtocol& protocol);
} // namespace facebook::fboss::thrift_cow
