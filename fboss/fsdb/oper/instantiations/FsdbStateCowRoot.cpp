/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/fsdb/oper/instantiations/FsdbCowRoot.h"

namespace facebook::fboss::thrift_cow {

template class ThriftStructNode<fsdb::FsdbOperStateRoot>;

} // namespace facebook::fboss::thrift_cow
