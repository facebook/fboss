// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/oper/instantiations/FsdbPathConverter.h"

namespace facebook::fboss::fsdb {

template class PathConverter<FsdbOperStateRoot>;

template class PathConverter<FsdbOperStatsRoot>;

} // namespace facebook::fboss::fsdb
