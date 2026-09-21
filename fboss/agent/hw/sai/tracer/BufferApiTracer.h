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

sai_buffer_api_t* wrappedBufferApi();

SET_ATTRIBUTE_FUNC_DECLARATION(BufferPool);
SET_ATTRIBUTE_FUNC_DECLARATION(BufferProfile);
SET_ATTRIBUTE_FUNC_DECLARATION(IngressPriorityGroup);

} // namespace facebook::fboss
