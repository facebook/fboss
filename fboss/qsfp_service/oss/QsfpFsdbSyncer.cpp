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

std::vector<std::string> QsfpFsdbSyncer::getStatePath() const {
  return {};
}

std::vector<std::string> QsfpFsdbSyncer::getStatsPath() const {
  return {};
}

} // namespace fboss
} // namespace facebook
