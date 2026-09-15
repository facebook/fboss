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
#include <fboss/fsdb/oper/instantiations/FsdbCowRoot.h>
#include "fboss/fsdb/if/FsdbModel.h"

namespace facebook::fboss::fsdb {

// Instantiated per member function rather than per class, so that the heavy
// members land in separate TUs instead of one. A whole-class instantiation
// expands every member into a single object, which peaks the compiler well
// above what one action should need. Each declaration below has a matching
// explicit instantiation in one of the FsdbCow{State,Stats}SubscriptionManager*
// TUs; keep the two in sync or the symbol goes missing at link time.

extern template void
CowSubscriptionManager<thrift_cow::FsdbCowStateRoot>::serveSubscriptions(
    SubscriptionStore&,
    const std::shared_ptr<thrift_cow::FsdbCowStateRoot>&,
    const std::shared_ptr<thrift_cow::FsdbCowStateRoot>&,
    const SubscriptionMetadataServer&);
extern template void
CowSubscriptionManager<thrift_cow::FsdbCowStateRoot>::pruneDeletedPaths(
    SubscriptionStore&,
    const std::shared_ptr<thrift_cow::FsdbCowStateRoot>&,
    const std::shared_ptr<thrift_cow::FsdbCowStateRoot>&);
extern template void
CowSubscriptionManager<thrift_cow::FsdbCowStateRoot>::publishAndAddPaths(
    SubscriptionStore&,
    std::shared_ptr<thrift_cow::FsdbCowStateRoot>&);
extern template void
CowSubscriptionManager<thrift_cow::FsdbCowStateRoot>::doInitialSync(
    SubscriptionStore&,
    const std::shared_ptr<thrift_cow::FsdbCowStateRoot>&,
    const SubscriptionMetadataServer&);

extern template void
CowSubscriptionManager<thrift_cow::FsdbCowStatsRoot>::serveSubscriptions(
    SubscriptionStore&,
    const std::shared_ptr<thrift_cow::FsdbCowStatsRoot>&,
    const std::shared_ptr<thrift_cow::FsdbCowStatsRoot>&,
    const SubscriptionMetadataServer&);
extern template void
CowSubscriptionManager<thrift_cow::FsdbCowStatsRoot>::pruneDeletedPaths(
    SubscriptionStore&,
    const std::shared_ptr<thrift_cow::FsdbCowStatsRoot>&,
    const std::shared_ptr<thrift_cow::FsdbCowStatsRoot>&);
extern template void
CowSubscriptionManager<thrift_cow::FsdbCowStatsRoot>::publishAndAddPaths(
    SubscriptionStore&,
    std::shared_ptr<thrift_cow::FsdbCowStatsRoot>&);
extern template void
CowSubscriptionManager<thrift_cow::FsdbCowStatsRoot>::doInitialSync(
    SubscriptionStore&,
    const std::shared_ptr<thrift_cow::FsdbCowStatsRoot>&,
    const SubscriptionMetadataServer&);

} // namespace facebook::fboss::fsdb
