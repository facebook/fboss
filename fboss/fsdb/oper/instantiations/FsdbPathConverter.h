// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/fsdb/oper/PathConverter.h"

namespace facebook::fboss::fsdb {

extern template class PathConverter<FsdbOperStateRoot>;

extern template class PathConverter<FsdbOperStatsRoot>;

} // namespace facebook::fboss::fsdb
