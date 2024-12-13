/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/VoqUtils.h"

#include "fboss/agent/AgentFeatures.h"

namespace facebook::fboss {

int getLocalPortNumVoqs(cfg::PortType portType, cfg::Scope portScope) {
  if (!isDualStage3Q2QMode()) {
    return 8;
  }
  if (FLAGS_dual_stage_edsw_3q_2q) {
    return 3;
  }
  CHECK(FLAGS_dual_stage_rdsw_3q_2q);
  if (portType == cfg::PortType::MANAGEMENT_PORT ||
      (portType == cfg::PortType::RECYCLE_PORT &&
       portScope == cfg::Scope::GLOBAL)) {
    return 2;
  }
  return 3;
}

int getRemotePortNumVoqs(
    HwAsic::InterfaceNodeRole intfRole,
    cfg::PortType portType) {
  if (intfRole == HwAsic::InterfaceNodeRole::DUAL_STAGE_EDGE_NODE) {
    CHECK(isDualStage3Q2QMode());
    return 3;
  }
  if (isDualStage3Q2QMode()) {
    if (portType == cfg::PortType::MANAGEMENT_PORT ||
        portType == cfg::PortType::RECYCLE_PORT) {
      return 2;
    }
    return 3;
  }
  return 8;
}
} // namespace facebook::fboss
