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

// Explicit instantiation for
// ThriftStructNode<FsdbOperStateRoot>::fromEncodedBuf
template void ThriftStructNode<fsdb::FsdbOperStateRoot>::fromEncodedBuf(
    fsdb::OperProtocol proto,
    folly::IOBuf&& encoded);

// Explicit instantiation for ThriftStructFields<FsdbOperStateRoot>
template struct ThriftStructFields<
    fsdb::FsdbOperStateRoot,
    ThriftStructNode<fsdb::FsdbOperStateRoot>>;

} // namespace facebook::fboss::thrift_cow
