/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/FirmwareApiTracer.h"

#include "fboss/agent/hw/sai/api/FirmwareApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)

using folly::to;

namespace {
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
std::map<int32_t, std::pair<std::string, std::size_t>> _FirmwareMap{
    SAI_ATTR_MAP(Firmware, Version),
    SAI_ATTR_MAP(Firmware, OpStatus),
    SAI_ATTR_MAP(Firmware, FunctionalStatus),
};
#endif

} // namespace

namespace facebook::fboss {

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
WRAP_CREATE_FUNC(
    firmware,
    static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_FIRMWARE),
    firmware);
WRAP_REMOVE_FUNC(
    firmware,
    static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_FIRMWARE),
    firmware);
WRAP_SET_ATTR_FUNC(
    firmware,
    static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_FIRMWARE),
    firmware);
WRAP_GET_ATTR_FUNC(
    firmware,
    static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_FIRMWARE),
    firmware);

sai_firmware_api_t* wrappedFirmwareApi() {
  static sai_firmware_api_t firmwareWrappers;

  firmwareWrappers.create_firmware = &wrap_create_firmware;
  firmwareWrappers.remove_firmware = &wrap_remove_firmware;
  firmwareWrappers.set_firmware_attribute = &wrap_set_firmware_attribute;
  firmwareWrappers.get_firmware_attribute = &wrap_get_firmware_attribute;

  return &firmwareWrappers;
}

SET_SAI_ATTRIBUTES(Firmware)
#endif

} // namespace facebook::fboss

#endif
