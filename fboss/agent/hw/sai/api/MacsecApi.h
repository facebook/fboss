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

#include "fboss/agent/hw/sai/api/SaiApi.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/api/Types.h"

#include <folly/logging/xlog.h>

#include <tuple>

extern "C" {
#include <sai.h>
}
namespace facebook::fboss {

class MacsecApi;

struct SaiMacsecTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_MACSEC;
  using SaiApiT = MacsecApi;
  struct Attributes {
    using EnumType = sai_macsec_attr_t;
    using Direction =
        SaiAttribute<EnumType, SAI_MACSEC_ATTR_DIRECTION, sai_int32_t>;
    using PhysicalBypass =
        SaiAttribute<EnumType, SAI_MACSEC_ATTR_PHYSICAL_BYPASS_ENABLE, bool>;
  };
  using AdapterKey = MacsecSaiId;
  using AdapterHostKey = Attributes::Direction;
  using CreateAttributes = std::
      tuple<Attributes::Direction, std::optional<Attributes::PhysicalBypass>>;
};

SAI_ATTRIBUTE_NAME(Macsec, Direction)
SAI_ATTRIBUTE_NAME(Macsec, PhysicalBypass)

struct SaiMacsecPortTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_MACSEC;
  using SaiApiT = MacsecApi;
  struct Attributes {
    using EnumType = sai_macsec_port_attr_t;
    using PortID =
        SaiAttribute<EnumType, SAI_MACSEC_PORT_ATTR_PORT_ID, sai_object_id_t>;
    using MacsecDirection = SaiAttribute<
        EnumType,
        SAI_MACSEC_PORT_ATTR_MACSEC_DIRECTION,
        sai_int32_t>;
  };

  using AdapterKey = MacsecPortSaiId;
  using AdapterHostKey =
      std::tuple<Attributes::PortID, Attributes::MacsecDirection>;
  using CreateAttributes =
      std::tuple<Attributes::PortID, Attributes::MacsecDirection>;
};

SAI_ATTRIBUTE_NAME(MacsecPort, PortID)
SAI_ATTRIBUTE_NAME(MacsecPort, MacsecDirection)

class MacsecApi : public SaiApi<MacsecApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_MACSEC;
  MacsecApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for macsec api");
  }

 private:
  sai_status_t _create(
      MacsecSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_macsec(rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _create(
      MacsecPortSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_macsec_port(rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(MacsecSaiId key) {
    return api_->remove_macsec(key);
  }

  sai_status_t _remove(MacsecPortSaiId key) {
    return api_->remove_macsec_port(key);
  }

  sai_status_t _getAttribute(MacsecSaiId key, sai_attribute_t* attr) const {
    return api_->get_macsec_attribute(key, 1, attr);
  }

  sai_status_t _getAttribute(MacsecPortSaiId key, sai_attribute_t* attr) const {
    return api_->get_macsec_port_attribute(key, 1, attr);
  }

  sai_status_t _setAttribute(MacsecSaiId key, const sai_attribute_t* attr) {
    return api_->set_macsec_attribute(key, attr);
  }

  sai_status_t _setAttribute(MacsecPortSaiId key, const sai_attribute_t* attr) {
    return api_->set_macsec_port_attribute(key, attr);
  }

  sai_macsec_api_t* api_;
  friend class SaiApi<MacsecApi>;
};

} // namespace facebook::fboss
