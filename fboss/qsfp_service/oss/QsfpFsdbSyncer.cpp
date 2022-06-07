/*
 *  Copyright (c) 2021-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/QsfpFsdbSyncer.h"

namespace facebook {
namespace fboss {

const std::vector<std::string>& QsfpFsdbSyncer::getStatePath() {
  static const std::vector<std::string> statePath;
  return statePath;
}

const std::vector<std::string>& QsfpFsdbSyncer::getStatsPath() {
  static const std::vector<std::string> statsPath;
  return statsPath;
}

const std::vector<std::string>& QsfpFsdbSyncer::getConfigPath() {
  static const std::vector<std::string> configPath;
  return configPath;
}

} // namespace fboss
} // namespace facebook
