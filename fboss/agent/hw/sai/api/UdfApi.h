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
#include "fboss/agent/hw/sai/api/Types.h"

#include <folly/logging/xlog.h>

#include <tuple>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class UdfApi;

struct SaiUdfTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_UDF;
  using SaiApiT = UdfApi;
  struct Attributes {
    using EnumType = sai_udf_attr_t;
    using UdfMatchId =
        SaiAttribute<EnumType, SAI_UDF_ATTR_MATCH_ID, SaiObjectIdT>;
    using UdfGroupId =
        SaiAttribute<EnumType, SAI_UDF_ATTR_GROUP_ID, SaiObjectIdT>;
    using Base = SaiAttribute<EnumType, SAI_UDF_ATTR_BASE, sai_int32_t>;
    using Offset = SaiAttribute<EnumType, SAI_UDF_ATTR_OFFSET, sai_uint16_t>;
  };

  using AdapterKey = UdfSaiId;
  using CreateAttributes = std::tuple<
      Attributes::UdfMatchId,
      Attributes::UdfGroupId,
      std::optional<Attributes::Base>,
      Attributes::Offset>;
  using AdapterHostKey = CreateAttributes;
};

SAI_ATTRIBUTE_NAME(Udf, UdfMatchId);
SAI_ATTRIBUTE_NAME(Udf, UdfGroupId);
SAI_ATTRIBUTE_NAME(Udf, Base);
SAI_ATTRIBUTE_NAME(Udf, Offset);

struct SaiUdfMatchTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_UDF_MATCH;
  using SaiApiT = UdfApi;
  struct Attributes {
    using EnumType = sai_udf_match_attr_t;
    using L2Type =
        SaiAttribute<EnumType, SAI_UDF_MATCH_ATTR_L2_TYPE, AclEntryFieldU16>;
    using L3Type =
        SaiAttribute<EnumType, SAI_UDF_MATCH_ATTR_L3_TYPE, AclEntryFieldU8>;
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
    using L4DstPortType = SaiAttribute<
        EnumType,
        SAI_UDF_MATCH_ATTR_L4_DST_PORT_TYPE,
        AclEntryFieldU16>;
#endif
  };

  using AdapterKey = UdfMatchSaiId;
  using CreateAttributes = std::tuple<
      Attributes::L2Type,
      Attributes::L3Type
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
      ,
      Attributes::L4DstPortType
#endif
      >;
  using AdapterHostKey = CreateAttributes;
};

SAI_ATTRIBUTE_NAME(UdfMatch, L2Type);
SAI_ATTRIBUTE_NAME(UdfMatch, L3Type);
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
SAI_ATTRIBUTE_NAME(UdfMatch, L4DstPortType);
#endif

struct SaiUdfGroupTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_UDF_GROUP;
  using SaiApiT = UdfApi;
  struct Attributes {
    using EnumType = sai_udf_group_attr_t;
    using Type = SaiAttribute<EnumType, SAI_UDF_GROUP_ATTR_TYPE, sai_int32_t>;
    using Length =
        SaiAttribute<EnumType, SAI_UDF_GROUP_ATTR_LENGTH, sai_uint16_t>;
  };

  using AdapterKey = UdfGroupSaiId;
  using CreateAttributes =
      std::tuple<std::optional<Attributes::Type>, Attributes::Length>;
  using AdapterHostKey = CreateAttributes;
};

SAI_ATTRIBUTE_NAME(UdfGroup, Type);
SAI_ATTRIBUTE_NAME(UdfGroup, Length);

class UdfApi : public SaiApi<UdfApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_UDF;
  UdfApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for udf api");
  }

  sai_status_t _create(
      UdfSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_udf(rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(UdfSaiId id) const {
    return api_->remove_udf(id);
  }

  sai_status_t _getAttribute(UdfSaiId id, sai_attribute_t* attr) const {
    return api_->get_udf_attribute(id, 1, attr);
  }

  sai_status_t _setAttribute(UdfSaiId id, const sai_attribute_t* attr) const {
    return api_->set_udf_attribute(id, attr);
  }

  sai_status_t _create(
      UdfMatchSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_udf_match(rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(UdfMatchSaiId id) const {
    return api_->remove_udf_match(id);
  }

  sai_status_t _getAttribute(UdfMatchSaiId id, sai_attribute_t* attr) const {
    return api_->get_udf_match_attribute(id, 1, attr);
  }

  sai_status_t _setAttribute(UdfMatchSaiId id, const sai_attribute_t* attr)
      const {
    return api_->set_udf_match_attribute(id, attr);
  }

  sai_status_t _create(
      UdfGroupSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_udf_group(rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(UdfGroupSaiId id) const {
    return api_->remove_udf_group(id);
  }

  sai_status_t _getAttribute(UdfGroupSaiId id, sai_attribute_t* attr) const {
    return api_->get_udf_group_attribute(id, 1, attr);
  }

  sai_status_t _setAttribute(UdfGroupSaiId id, const sai_attribute_t* attr)
      const {
    return api_->set_udf_group_attribute(id, attr);
  }

  sai_udf_api_t* api_;
  friend class SaiApi<UdfApi>;
};

} // namespace facebook::fboss
