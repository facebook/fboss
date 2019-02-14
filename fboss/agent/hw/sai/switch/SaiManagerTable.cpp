/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"

#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"

namespace facebook {
namespace fboss {

SaiManagerTable::SaiManagerTable(SaiApiTable* apiTable) : apiTable_(apiTable) {
  portManager_ = std::make_unique<SaiPortManager>(apiTable_, this);
}
SaiManagerTable::~SaiManagerTable() {}

SaiPortManager& SaiManagerTable::portManager() {
  return *portManager_;
}
const SaiPortManager& SaiManagerTable::portManager() const {
  return *portManager_;
}

} // namespace fboss
} // namespace facebook
