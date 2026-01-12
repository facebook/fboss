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

#include "fboss/agent/types.h"

#include <vector>
namespace facebook::fboss {

class TestEnsembleIf;
class SwitchState;

namespace utility {

std::vector<PortID> getHyperPortMembers(
    const std::shared_ptr<SwitchState>& state,
    PortID port);

} // namespace utility
} // namespace facebook::fboss
