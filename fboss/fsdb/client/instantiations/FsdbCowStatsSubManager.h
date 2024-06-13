// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/client/FsdbSubManager.h"
#include "fboss/fsdb/if/FsdbModel.h"

namespace facebook::fboss::fsdb {

extern template class FsdbSubManager<FsdbOperStatsRoot, true /* IsCow */>;
using FsdbCowStatsSubManager =
    FsdbSubManager<FsdbOperStatsRoot, true /* IsCow */>;

} // namespace facebook::fboss::fsdb
