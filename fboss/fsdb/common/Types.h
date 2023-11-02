// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"

namespace facebook::fboss::fsdb {

using FsdbExceptionCallback = std::function<void(const FsdbException&)>;
using Callback = std::function<void()>;

} // namespace facebook::fboss::fsdb
