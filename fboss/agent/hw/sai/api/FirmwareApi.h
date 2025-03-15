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

#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)

#include "fboss/agent/hw/sai/api/SaiApi.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/api/Types.h"

#include <folly/logging/xlog.h>

#include <tuple>

extern "C" {
#include <experimental/saiexperimentalfirmware.h>
#include <sai.h>
#include <saiextensions.h>

#ifndef IS_OSS_BRCM_SAI
#include <experimental/saiexperimentalfirmware.h>
#else
#include <saiexperimentalfirmware.h>
#endif
}

namespace facebook::fboss {

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
class FirmwareApi;

struct SaiFirmwareTraits {
  static constexpr sai_object_type_t ObjectType =
      static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_FIRMWARE);
  using SaiApiT = FirmwareApi;
  struct Attributes {
    using EnumType = sai_firmware_attr_t;
    using Version = SaiAttribute<
        EnumType,
        SAI_FIRMWARE_ATTR_VERSION,
        std::vector<int8_t>,
        SaiS8ListDefault>;
  };

  using AdapterKey = FirmwareSaiId;
  using AdapterHostKey = std::monostate;
  using CreateAttributes = std::tuple<Attributes::Version>;
};

SAI_ATTRIBUTE_NAME(Firmware, Version)

class FirmwareApi : public SaiApi<FirmwareApi> {
 public:
  static constexpr sai_api_t ApiType = static_cast<sai_api_t>(SAI_API_FIRMWARE);
  FirmwareApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for firmware api");
  }
  FirmwareApi(const FirmwareApi& other) = delete;
  FirmwareApi& operator=(const FirmwareApi& other) = delete;

 private:
  sai_status_t _create(
      FirmwareSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_firmware(rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _remove(FirmwareSaiId id) const {
    return api_->remove_firmware(id);
  }
  sai_status_t _getAttribute(FirmwareSaiId id, sai_attribute_t* attr) const {
    return api_->get_firmware_attribute(id, 1, attr);
  }
  sai_status_t _setAttribute(FirmwareSaiId id, const sai_attribute_t* attr)
      const {
    return api_->set_firmware_attribute(id, attr);
  }

  sai_firmware_api_t* api_;
  friend class SaiApi<FirmwareApi>;
};
#endif

} // namespace facebook::fboss

#endif
