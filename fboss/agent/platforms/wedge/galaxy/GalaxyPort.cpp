/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/wedge/galaxy/GalaxyPort.h"

#include "fboss/agent/platforms/wedge/galaxy/GalaxyPlatform.h"

namespace facebook::fboss {

GalaxyPort::GalaxyPort(
    PortID id,
    GalaxyPlatform* platform,
    std::optional<FrontPanelResources> frontPanel)
    : WedgePort(id, platform, frontPanel) {}

} // namespace facebook::fboss
