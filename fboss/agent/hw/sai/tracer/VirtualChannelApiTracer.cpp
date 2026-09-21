/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/VirtualChannelApiTracer.h" // NOLINT(facebook-unused-include-check)

#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/VirtualChannelApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

#if defined(BRCM_SAI_SDK_XGS_GTE_16_0)

using folly::to;

namespace {

std::map<int32_t, std::pair<std::string, std::size_t>> _VirtualChannelMap{
    SAI_ATTR_MAP(VirtualChannel, Port),
    SAI_ATTR_MAP(VirtualChannel, Index),
};

} // namespace

namespace facebook::fboss {

WRAP_CREATE_FUNC(
    virtual_channel,
    SAI_OBJECT_TYPE_VIRTUAL_CHANNEL,
    virtualChannel);
WRAP_REMOVE_FUNC(
    virtual_channel,
    SAI_OBJECT_TYPE_VIRTUAL_CHANNEL,
    virtualChannel);
WRAP_SET_ATTR_FUNC(
    virtual_channel,
    SAI_OBJECT_TYPE_VIRTUAL_CHANNEL,
    virtualChannel);
WRAP_GET_ATTR_FUNC(
    virtual_channel,
    SAI_OBJECT_TYPE_VIRTUAL_CHANNEL,
    virtualChannel);

sai_virtual_channel_api_t* wrappedVirtualChannelApi() {
  static sai_virtual_channel_api_t virtualChannelWrappers;

  virtualChannelWrappers.create_virtual_channel = &wrap_create_virtual_channel;
  virtualChannelWrappers.remove_virtual_channel = &wrap_remove_virtual_channel;
  virtualChannelWrappers.set_virtual_channel_attribute =
      &wrap_set_virtual_channel_attribute;
  virtualChannelWrappers.get_virtual_channel_attribute =
      &wrap_get_virtual_channel_attribute;

  return &virtualChannelWrappers;
}

SET_SAI_ATTRIBUTES(VirtualChannel)

} // namespace facebook::fboss

#endif
