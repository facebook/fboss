/*
 *  Copyright (c) 2022-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/fsdb/client/FsdbSyncManager.h"

namespace facebook {
namespace fboss {

class QsfpFsdbSyncManager : public facebook::fboss::fsdb::FsdbSyncManager {
 public:
  QsfpFsdbSyncManager();

  static std::vector<std::string> getStatePath();
  static std::vector<std::string> getStatsPath();
  static std::vector<std::string> getConfigPath();
};

} // namespace fboss
} // namespace facebook
