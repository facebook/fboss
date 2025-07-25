/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/HwTestThriftHandler.h"

namespace facebook::fboss::utility {
int HwTestThriftHandler::getNumTeFlowEntries() {
  // This function is not implemented yet.
  // If the test is running on SAI Switches,
  // it should throw an error.
  return 0;
}

bool HwTestThriftHandler::checkSwHwTeFlowMatch(
    std::unique_ptr<::facebook::fboss::state::TeFlowEntryFields> /*Unused*/) {
  // This function is not implemented yet.
  // If the test is running on SAI Switches,
  // it should throw an error.
  return false;
}

} // namespace facebook::fboss::utility
