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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {

constexpr auto k2StageEdgePodClusterId = 200;

int getLocalPortNumVoqs(cfg::PortType portType, cfg::Scope portScope);
int getRemotePortNumVoqs(
    HwAsic::InterfaceNodeRole intfRole,
    cfg::PortType portType);

} // namespace facebook::fboss
