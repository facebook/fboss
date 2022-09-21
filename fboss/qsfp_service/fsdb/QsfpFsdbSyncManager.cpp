/*
 *  Copyright (c) 2021-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/fsdb/QsfpFsdbSyncManager.h"

namespace facebook {
namespace fboss {

QsfpFsdbSyncManager::QsfpFsdbSyncManager()
    : fsdb::FsdbSyncManager("qsfp_service", getStatePath(), getStatsPath()) {}

} // namespace fboss
} // namespace facebook
