/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/MacTableManager.h"

#include "fboss/agent/L2Entry.h"
#include "fboss/agent/MacTableUtils.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

MacTableManager::MacTableManager(SwSwitch* sw) : sw_(sw) {}

void MacTableManager::handleL2LearningUpdate(
    L2Entry l2Entry,
    L2EntryUpdateType l2EntryUpdateType) {
  auto updateMacTableFn =
      [l2Entry, l2EntryUpdateType](const std::shared_ptr<SwitchState>& state) {
        return MacTableUtils::updateMacTable(state, l2Entry, l2EntryUpdateType);
      };

  sw_->updateState(
      folly::to<std::string>("Programming : ", l2Entry.str()),
      std::move(updateMacTableFn));
}

} // namespace facebook::fboss
