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

#include "fboss/agent/hw/sai/tracer/SaiTracer.h"

namespace facebook::fboss {

sai_hostif_api_t* wrappedHostifApi();

SET_ATTRIBUTE_FUNC_DECLARATION(HostifTrap);
SET_ATTRIBUTE_FUNC_DECLARATION(HostifUserDefinedTrap);
SET_ATTRIBUTE_FUNC_DECLARATION(HostifTrapGroup);
SET_ATTRIBUTE_FUNC_DECLARATION(HostifPacket);

} // namespace facebook::fboss
