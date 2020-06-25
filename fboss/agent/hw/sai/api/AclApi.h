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

class AclApi;

struct SaiAclTableGroupTraits {
  static constexpr sai_object_type_t ObjectType =
      SAI_OBJECT_TYPE_ACL_TABLE_GROUP;
  using SaiApiT = AclApi;
  struct Attributes {
    using EnumType = sai_acl_table_group_attr_t;

    using Stage =
        SaiAttribute<EnumType, SAI_ACL_TABLE_GROUP_ATTR_ACL_STAGE, sai_int32_t>;
    using BindPointTypeList = SaiAttribute<
        EnumType,
        SAI_ACL_TABLE_GROUP_ATTR_ACL_BIND_POINT_TYPE_LIST,
        std::vector<sai_int32_t>>;
    using Type =
        SaiAttribute<EnumType, SAI_ACL_TABLE_GROUP_ATTR_TYPE, sai_int32_t>;
    using MemberList = SaiAttribute<
        EnumType,
        SAI_ACL_TABLE_GROUP_ATTR_MEMBER_LIST,
        std::vector<sai_object_id_t>>;
  };

  using AdapterKey = AclTableGroupSaiId;
  using AdapterHostKey = std::tuple<
      Attributes::Stage,
      std::optional<Attributes::BindPointTypeList>,
      std::optional<Attributes::Type>>;
  using CreateAttributes = std::tuple<
      Attributes::Stage,
      std::optional<Attributes::BindPointTypeList>,
      std::optional<Attributes::Type>>;
};

SAI_ATTRIBUTE_NAME(AclTableGroup, Stage);
SAI_ATTRIBUTE_NAME(AclTableGroup, BindPointTypeList);
SAI_ATTRIBUTE_NAME(AclTableGroup, Type);
SAI_ATTRIBUTE_NAME(AclTableGroup, MemberList);

struct SaiAclTableGroupMemberTraits {
  static constexpr sai_object_type_t ObjectType =
      SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER;
  using SaiApiT = AclApi;
  struct Attributes {
    using EnumType = sai_acl_table_group_member_attr_t;

    using TableGroupId = SaiAttribute<
        EnumType,
        SAI_ACL_TABLE_GROUP_MEMBER_ATTR_ACL_TABLE_GROUP_ID,
        SaiObjectIdT>;
    using TableId = SaiAttribute<
        EnumType,
        SAI_ACL_TABLE_GROUP_MEMBER_ATTR_ACL_TABLE_ID,
        SaiObjectIdT>;
    using Priority = SaiAttribute<
        EnumType,
        SAI_ACL_TABLE_GROUP_MEMBER_ATTR_PRIORITY,
        sai_uint32_t>;
  };

  using AdapterKey = AclTableGroupMemberSaiId;
  using AdapterHostKey = std::tuple<
      Attributes::TableGroupId,
      Attributes::TableId,
      Attributes::Priority>;
  using CreateAttributes = std::tuple<
      Attributes::TableGroupId,
      Attributes::TableId,
      Attributes::Priority>;
};

SAI_ATTRIBUTE_NAME(AclTableGroupMember, TableGroupId);
SAI_ATTRIBUTE_NAME(AclTableGroupMember, TableId);
SAI_ATTRIBUTE_NAME(AclTableGroupMember, Priority);

struct SaiAclTableTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_ACL_TABLE;
  using SaiApiT = AclApi;
  struct Attributes {
    using EnumType = sai_acl_table_attr_t;

    using Stage =
        SaiAttribute<EnumType, SAI_ACL_TABLE_ATTR_ACL_STAGE, sai_int32_t>;
    using BindPointTypeList = SaiAttribute<
        EnumType,
        SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST,
        std::vector<sai_int32_t>>;
    using ActionTypeList = SaiAttribute<
        EnumType,
        SAI_ACL_TABLE_ATTR_ACL_ACTION_TYPE_LIST,
        std::vector<sai_int32_t>>;
    using EntryList = SaiAttribute<
        EnumType,
        SAI_ACL_TABLE_ATTR_ENTRY_LIST,
        std::vector<sai_object_id_t>>;

    /*
     * At least one Field* must be set.
     */
    using FieldSrcIpV6 =
        SaiAttribute<EnumType, SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6, bool>;
    using FieldDstIpV6 =
        SaiAttribute<EnumType, SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6, bool>;
    using FieldL4SrcPort =
        SaiAttribute<EnumType, SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT, bool>;
    using FieldL4DstPort =
        SaiAttribute<EnumType, SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT, bool>;
    using FieldIpProtocol =
        SaiAttribute<EnumType, SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL, bool>;
    using FieldTcpFlags =
        SaiAttribute<EnumType, SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS, bool>;
    using FieldInPort =
        SaiAttribute<EnumType, SAI_ACL_TABLE_ATTR_FIELD_IN_PORT, bool>;
    using FieldOutPort =
        SaiAttribute<EnumType, SAI_ACL_TABLE_ATTR_FIELD_OUT_PORT, bool>;
    using FieldIpFrag =
        SaiAttribute<EnumType, SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_FRAG, bool>;
    using FieldDscp =
        SaiAttribute<EnumType, SAI_ACL_TABLE_ATTR_FIELD_DSCP, bool>;
    using FieldDstMac =
        SaiAttribute<EnumType, SAI_ACL_TABLE_ATTR_FIELD_DST_MAC, bool>;
    using FieldIpType =
        SaiAttribute<EnumType, SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE, bool>;
    using FieldTtl = SaiAttribute<EnumType, SAI_ACL_TABLE_ATTR_FIELD_TTL, bool>;
  };

  using AdapterKey = AclTableSaiId;
  using AdapterHostKey = std::tuple<
      Attributes::Stage,
      std::optional<Attributes::BindPointTypeList>,
      std::optional<Attributes::ActionTypeList>,
      std::optional<Attributes::FieldSrcIpV6>,
      std::optional<Attributes::FieldDstIpV6>,
      std::optional<Attributes::FieldL4SrcPort>,
      std::optional<Attributes::FieldL4DstPort>,
      std::optional<Attributes::FieldIpProtocol>,
      std::optional<Attributes::FieldTcpFlags>,
      std::optional<Attributes::FieldInPort>,
      std::optional<Attributes::FieldOutPort>,
      std::optional<Attributes::FieldIpFrag>,
      std::optional<Attributes::FieldDscp>,
      std::optional<Attributes::FieldDstMac>,
      std::optional<Attributes::FieldIpType>,
      std::optional<Attributes::FieldTtl>>;

  using CreateAttributes = std::tuple<
      Attributes::Stage,
      std::optional<Attributes::BindPointTypeList>,
      std::optional<Attributes::ActionTypeList>,
      std::optional<Attributes::FieldSrcIpV6>,
      std::optional<Attributes::FieldDstIpV6>,
      std::optional<Attributes::FieldL4SrcPort>,
      std::optional<Attributes::FieldL4DstPort>,
      std::optional<Attributes::FieldIpProtocol>,
      std::optional<Attributes::FieldTcpFlags>,
      std::optional<Attributes::FieldInPort>,
      std::optional<Attributes::FieldOutPort>,
      std::optional<Attributes::FieldIpFrag>,
      std::optional<Attributes::FieldDscp>,
      std::optional<Attributes::FieldDstMac>,
      std::optional<Attributes::FieldIpType>,
      std::optional<Attributes::FieldTtl>>;
};

SAI_ATTRIBUTE_NAME(AclTable, Stage);
SAI_ATTRIBUTE_NAME(AclTable, BindPointTypeList);
SAI_ATTRIBUTE_NAME(AclTable, ActionTypeList);
SAI_ATTRIBUTE_NAME(AclTable, EntryList);
SAI_ATTRIBUTE_NAME(AclTable, FieldSrcIpV6);
SAI_ATTRIBUTE_NAME(AclTable, FieldDstIpV6);
SAI_ATTRIBUTE_NAME(AclTable, FieldL4SrcPort);
SAI_ATTRIBUTE_NAME(AclTable, FieldL4DstPort);
SAI_ATTRIBUTE_NAME(AclTable, FieldIpProtocol);
SAI_ATTRIBUTE_NAME(AclTable, FieldTcpFlags);
SAI_ATTRIBUTE_NAME(AclTable, FieldInPort);
SAI_ATTRIBUTE_NAME(AclTable, FieldOutPort);
SAI_ATTRIBUTE_NAME(AclTable, FieldIpFrag);
SAI_ATTRIBUTE_NAME(AclTable, FieldDscp);
SAI_ATTRIBUTE_NAME(AclTable, FieldDstMac);
SAI_ATTRIBUTE_NAME(AclTable, FieldIpType);
SAI_ATTRIBUTE_NAME(AclTable, FieldTtl);

struct SaiAclEntryTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_ACL_ENTRY;
  using SaiApiT = AclApi;
  struct Attributes {
    using EnumType = sai_acl_entry_attr_t;

    using TableId =
        SaiAttribute<EnumType, SAI_ACL_ENTRY_ATTR_TABLE_ID, sai_object_id_t>;
    using Priority =
        SaiAttribute<EnumType, SAI_ACL_ENTRY_ATTR_PRIORITY, sai_uint32_t>;
    /*
     * TODO (skhare) Add all the FIELD_* FBOSS needs.
     * Find a way to express at least one FIELD_* is mandatory.
     */
    using FieldSrcIpV6 = SaiAttribute<
        EnumType,
        SAI_ACL_ENTRY_ATTR_FIELD_SRC_IPV6,
        AclEntryFieldIpV6>;
    using FieldDstIpV6 = SaiAttribute<
        EnumType,
        SAI_ACL_ENTRY_ATTR_FIELD_DST_IPV6,
        AclEntryFieldIpV6>;
    using FieldDscp =
        SaiAttribute<EnumType, SAI_ACL_ENTRY_ATTR_FIELD_DSCP, AclEntryFieldU8>;
    using FieldRouteDstUserMeta = SaiAttribute<
        EnumType,
        SAI_ACL_ENTRY_ATTR_FIELD_ROUTE_DST_USER_META,
        AclEntryFieldU32>;
  };

  using AdapterKey = AclEntrySaiId;
  using AdapterHostKey = std::tuple<
      Attributes::TableId,
      std::optional<Attributes::Priority>,
      std::optional<Attributes::FieldSrcIpV6>,
      std::optional<Attributes::FieldDstIpV6>,
      std::optional<Attributes::FieldDscp>,
      std::optional<Attributes::FieldRouteDstUserMeta>>;
  using CreateAttributes = std::tuple<
      Attributes::TableId,
      std::optional<Attributes::Priority>,
      std::optional<Attributes::FieldSrcIpV6>,
      std::optional<Attributes::FieldDstIpV6>,
      std::optional<Attributes::FieldDscp>,
      std::optional<Attributes::FieldRouteDstUserMeta>>;
};

SAI_ATTRIBUTE_NAME(AclEntry, TableId);
SAI_ATTRIBUTE_NAME(AclEntry, Priority);
SAI_ATTRIBUTE_NAME(AclEntry, FieldSrcIpV6);
SAI_ATTRIBUTE_NAME(AclEntry, FieldDstIpV6);
SAI_ATTRIBUTE_NAME(AclEntry, FieldDscp);
SAI_ATTRIBUTE_NAME(AclEntry, FieldRouteDstUserMeta);

class AclApi : public SaiApi<AclApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_ACL;
  AclApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for Acl api");
  }
  AclApi(const AclApi& other) = delete;

 private:
  sai_status_t _create(
      AclTableGroupSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_acl_table_group(
        rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _create(
      AclTableGroupMemberSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_acl_table_group_member(
        rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _create(
      AclTableSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_acl_table(rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _create(
      AclEntrySaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_acl_entry(rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(AclTableGroupSaiId id) {
    return api_->remove_acl_table_group(id);
  }

  sai_status_t _remove(AclTableGroupMemberSaiId id) {
    return api_->remove_acl_table_group_member(id);
  }

  sai_status_t _remove(AclTableSaiId id) {
    return api_->remove_acl_table(id);
  }

  sai_status_t _remove(AclEntrySaiId id) {
    return api_->remove_acl_entry(id);
  }

  sai_status_t _getAttribute(AclTableGroupSaiId id, sai_attribute_t* attr)
      const {
    return api_->get_acl_table_group_attribute(id, 1, attr);
  }

  sai_status_t _getAttribute(AclTableGroupMemberSaiId id, sai_attribute_t* attr)
      const {
    return api_->get_acl_table_group_member_attribute(id, 1, attr);
  }

  sai_status_t _getAttribute(AclTableSaiId id, sai_attribute_t* attr) const {
    return api_->get_acl_table_attribute(id, 1, attr);
  }

  sai_status_t _getAttribute(AclEntrySaiId id, sai_attribute_t* attr) const {
    return api_->get_acl_entry_attribute(id, 1, attr);
  }

  sai_status_t _setAttribute(AclTableGroupSaiId id, const sai_attribute_t* attr)
      const {
    return api_->set_acl_table_group_attribute(id, attr);
  }

  sai_status_t _setAttribute(
      AclTableGroupMemberSaiId id,
      const sai_attribute_t* attr) const {
    return api_->set_acl_table_group_member_attribute(id, attr);
  }

  sai_status_t _setAttribute(AclTableSaiId id, const sai_attribute_t* attr)
      const {
    return api_->set_acl_table_attribute(id, attr);
  }

  sai_status_t _setAttribute(AclEntrySaiId id, const sai_attribute_t* attr)
      const {
    return api_->set_acl_entry_attribute(id, attr);
  }

  sai_acl_api_t* api_;
  friend class SaiApi<AclApi>;
};

} // namespace facebook::fboss
