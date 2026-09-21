/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/tracer/SamplePacketApiTracer.h"
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/SamplePacketApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _SamplePacketMap{
    SAI_ATTR_MAP(SamplePacket, SampleRate),
    SAI_ATTR_MAP(SamplePacket, Type),
    SAI_ATTR_MAP(SamplePacket, Mode),
};
} // namespace

namespace facebook::fboss {

WRAP_CREATE_FUNC(samplepacket, SAI_OBJECT_TYPE_SAMPLEPACKET, samplepacket);
WRAP_REMOVE_FUNC(samplepacket, SAI_OBJECT_TYPE_SAMPLEPACKET, samplepacket);
WRAP_SET_ATTR_FUNC(samplepacket, SAI_OBJECT_TYPE_SAMPLEPACKET, samplepacket);
WRAP_GET_ATTR_FUNC(samplepacket, SAI_OBJECT_TYPE_SAMPLEPACKET, samplepacket);

sai_samplepacket_api_t* wrappedSamplePacketApi() {
  static sai_samplepacket_api_t samplepacketWrappers;
  samplepacketWrappers.create_samplepacket = &wrap_create_samplepacket;
  samplepacketWrappers.remove_samplepacket = &wrap_remove_samplepacket;
  samplepacketWrappers.set_samplepacket_attribute =
      &wrap_set_samplepacket_attribute;
  samplepacketWrappers.get_samplepacket_attribute =
      &wrap_get_samplepacket_attribute;
  return &samplepacketWrappers;
}

SET_SAI_ATTRIBUTES(SamplePacket)

} // namespace facebook::fboss
