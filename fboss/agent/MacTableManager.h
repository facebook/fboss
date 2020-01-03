/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/L2Entry.h"

namespace facebook::fboss {

class SwSwitch;

class MacTableManager {
 public:
  explicit MacTableManager(SwSwitch* sw);

  void handleL2LearningUpdate(
      L2Entry l2Entry,
      L2EntryUpdateType l2EntryUpdateType);

 private:
  // Forbidden copy constructor and assignment operator
  MacTableManager(MacTableManager const&) = delete;
  MacTableManager& operator=(MacTableManager const&) = delete;

  SwSwitch* sw_{nullptr};
};

} // namespace facebook::fboss
