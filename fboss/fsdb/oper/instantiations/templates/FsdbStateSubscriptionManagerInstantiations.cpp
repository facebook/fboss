/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/fsdb/oper/instantiations/templates/FsdbStateSubscriptionManagerInstantiations.h"

namespace facebook::fboss::fsdb {

// Explicit instantiation for SubscriptionManager::serveSubscriptions
template class SubscriptionManager<
    thrift_cow::ThriftStructNode<fsdb::FsdbOperStateRoot>,
    CowSubscriptionManager<thrift_cow::FsdbCowStateRoot>>;

} // namespace facebook::fboss::fsdb
