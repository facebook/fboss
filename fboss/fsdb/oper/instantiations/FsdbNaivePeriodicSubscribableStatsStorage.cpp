// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/oper/instantiations/FsdbNaivePeriodicSubscribableStorage.h"

namespace facebook::fboss::fsdb {

template class NaivePeriodicSubscribableStorage<
    CowStorage<FsdbOperStatsRoot>,
    CowSubscriptionManager<thrift_cow::FsdbCowStatsRoot>>;

} // namespace facebook::fboss::fsdb
