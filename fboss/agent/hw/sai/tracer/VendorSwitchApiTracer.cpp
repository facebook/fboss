/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/VendorSwitchApiTracer.h"
#include "fboss/agent/hw/sai/api/VendorSwitchApi.h"

#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
using folly::to;

namespace {

std::map<int32_t, std::pair<std::string, std::size_t>> _VendorSwitchMap{
    SAI_ATTR_MAP(VendorSwitch, EventList),
    SAI_ATTR_MAP(VendorSwitch, DisableEventList),
    SAI_ATTR_MAP(VendorSwitch, EnableEventList),
    SAI_ATTR_MAP(VendorSwitch, CgmRejectStatusBitmap),
};

void handleExtensionAttributes() {}

} // namespace

namespace facebook::fboss {

WRAP_CREATE_FUNC(
    vendor_switch,
    static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_VENDOR_SWITCH),
    vendorSwitch);
WRAP_REMOVE_FUNC(
    vendor_switch,
    static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_VENDOR_SWITCH),
    vendorSwitch);
WRAP_SET_ATTR_FUNC(
    vendor_switch,
    static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_VENDOR_SWITCH),
    vendorSwitch);
WRAP_GET_ATTR_FUNC(
    vendor_switch,
    static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_VENDOR_SWITCH),
    vendorSwitch);

sai_vendor_switch_api_t* wrappedVendorSwitchApi() {
  handleExtensionAttributes();
  static sai_vendor_switch_api_t vendorSwitchWrappers;

  vendorSwitchWrappers.create_vendor_switch = &wrap_create_vendor_switch;
  vendorSwitchWrappers.remove_vendor_switch = &wrap_remove_vendor_switch;
  vendorSwitchWrappers.set_vendor_switch_attribute =
      &wrap_set_vendor_switch_attribute;
  vendorSwitchWrappers.get_vendor_switch_attribute =
      &wrap_get_vendor_switch_attribute;

  return &vendorSwitchWrappers;
}

SET_SAI_ATTRIBUTES(VendorSwitch)

} // namespace facebook::fboss

#endif
