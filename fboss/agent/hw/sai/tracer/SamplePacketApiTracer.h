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

namespace facebook::fboss {

sai_samplepacket_api_t* wrappedSamplePacketApi();

SET_ATTRIBUTE_FUNC_DECLARATION(SamplePacket);

} // namespace facebook::fboss
