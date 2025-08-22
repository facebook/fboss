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

#include <fboss/fsdb/oper/CowSubscriptionManager.h>
#include <fboss/fsdb/oper/SubscriptionManager.h>
#include <fboss/fsdb/oper/instantiations/FsdbCowRoot.h>
#include <fboss/thrift_cow/nodes/ThriftStructNode-inl.h>

namespace facebook::fboss::fsdb {

// Extern template for SubscriptionManager::serveSubscriptions
extern template class SubscriptionManager<
    thrift_cow::FsdbCowStateRoot,
    CowSubscriptionManager<thrift_cow::FsdbCowStateRoot>>;

} // namespace facebook::fboss::fsdb
