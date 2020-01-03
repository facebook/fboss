/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <memory>

namespace facebook::fboss {

class SwitchState;

std::shared_ptr<SwitchState> setupAlpmState(
    std::shared_ptr<SwitchState> curState);
/*
 * Minimum required ALPM State
 */
std::shared_ptr<SwitchState> getMinimumAlpmState();

uint64_t numMinAlpmRoutes();
uint64_t numMinAlpmV4Routes();
uint64_t numMinAlpmV6Routes();

} // namespace facebook::fboss
