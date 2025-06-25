/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/test/MockStateMachine.h"
#include "fboss/agent/FbossError.h"

namespace facebook::fboss {

MockState getMockStateByOrder(int currentStateOrder) {
  switch (currentStateOrder) {
    case 0:
      return MockState::STATE_1;
    case 1:
      return MockState::STATE_2;
    case 2:
      return MockState::STATE_3;
    default:
      throw FbossError("Unsupported MockState order: ", currentStateOrder);
  }
}
} // namespace facebook::fboss
