// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/oper/NaivePeriodicSubscribableStorage.h"

namespace facebook::fboss::fsdb {

DEFINE_bool(
    serveHeartbeats,
    false,
    "Whether or not to serve hearbeats in subscription streams");
} // namespace facebook::fboss::fsdb
