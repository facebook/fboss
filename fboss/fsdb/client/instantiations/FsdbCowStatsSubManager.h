// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/client/FsdbSubManager.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/fsdb/oper/instantiations/FsdbCowStorage.h"

namespace facebook::fboss::fsdb {

extern template class FsdbSubManager<
    CowStorage<FsdbOperStatsRoot>,
    true /* IsCow */>;
using FsdbCowStatsSubManager =
    FsdbSubManager<CowStorage<FsdbOperStatsRoot>, true /* IsCow */>;

} // namespace facebook::fboss::fsdb
