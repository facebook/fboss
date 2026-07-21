/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/SaiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

sai_port_api_t* wrappedPortApi();

SET_ATTRIBUTE_FUNC_DECLARATION(Port);
SET_ATTRIBUTE_FUNC_DECLARATION(PortSerdes);
SET_ATTRIBUTE_FUNC_DECLARATION(PortConnector);
#if SAI_API_VERSION >= SAI_VERSION(1, 18, 0)
SET_ATTRIBUTE_FUNC_DECLARATION(PortLlrProfile);
#endif

} // namespace facebook::fboss
