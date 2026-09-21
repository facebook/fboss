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
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_MACSEC_PORT;
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
  static constexpr std::array<sai_stat_id_t, 3> CounterIdsToRead = {
      SAI_MACSEC_PORT_STAT_PRE_MACSEC_DROP_PKTS,
      SAI_MACSEC_PORT_STAT_CONTROL_PKTS,
      SAI_MACSEC_PORT_STAT_DATA_PKTS,
  };
  static constexpr std::array<sai_stat_id_t, 0> CounterIdsToReadAndClear = {};
};

SAI_ATTRIBUTE_NAME(MacsecPort, PortID)
SAI_ATTRIBUTE_NAME(MacsecPort, MacsecDirection)

template <>
struct SaiObjectHasStats<SaiMacsecPortTraits> : public std::true_type {};

struct SaiMacsecSATraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_MACSEC_SA;
  using SaiApiT = MacsecApi;
  struct Attributes {
    using EnumType = sai_macsec_sa_attr_t;
    using SCID =
        SaiAttribute<EnumType, SAI_MACSEC_SA_ATTR_SC_ID, sai_object_id_t>;
    using AssocNum = SaiAttribute<EnumType, SAI_MACSEC_SA_ATTR_AN, sai_uint8_t>;
    using AuthKey = SaiAttribute<
        EnumType,
        SAI_MACSEC_SA_ATTR_AUTH_KEY,
        std::array<uint8_t, sizeof(sai_macsec_auth_key_t)>>;
    using MacsecDirection = SaiAttribute<
        EnumType,
        SAI_MACSEC_SA_ATTR_MACSEC_DIRECTION,
        sai_int32_t>;
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
    using SSCI =
        SaiAttribute<EnumType, SAI_MACSEC_SA_ATTR_MACSEC_SSCI, sai_uint32_t>;
#endif
    using MinimumXpn =
        SaiAttribute<EnumType, SAI_MACSEC_SA_ATTR_MINIMUM_XPN, sai_uint64_t>;
    using SAK = SaiAttribute<
        EnumType,
        SAI_MACSEC_SA_ATTR_SAK,
        std::array<uint8_t, sizeof(sai_macsec_sak_t)>>;
    using Salt = SaiAttribute<
        EnumType,
        SAI_MACSEC_SA_ATTR_SALT,
        std::array<uint8_t, sizeof(sai_macsec_salt_t)>>;
#if SAI_API_VERSION >= SAI_VERSION(1, 8, 1)
    using CurrentXpn =
        SaiAttribute<EnumType, SAI_MACSEC_SA_ATTR_CURRENT_XPN, sai_uint64_t>;
#endif
  };

  using AdapterKey = MacsecSASaiId;
  using AdapterHostKey = std::tuple<
      Attributes::SCID,
      Attributes::AssocNum,
      Attributes::MacsecDirection>;
  using CreateAttributes = std::tuple<
      Attributes::SCID,
      Attributes::AssocNum,
      Attributes::AuthKey,
      Attributes::MacsecDirection,
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
      Attributes::SSCI,
#endif
      Attributes::SAK,
      Attributes::Salt,
      std::optional<Attributes::MinimumXpn>>;

  // TODO(ccpowers): in our first implementation (without sai wrappers), we read
  // different stats depending on the direction. Is it OK to just read them all?
  static constexpr std::array<sai_stat_id_t, 12> CounterIdsToRead = {
      SAI_MACSEC_SA_STAT_OCTETS_ENCRYPTED,
      SAI_MACSEC_SA_STAT_OCTETS_PROTECTED,
      SAI_MACSEC_SA_STAT_OUT_PKTS_ENCRYPTED,
      SAI_MACSEC_SA_STAT_OUT_PKTS_PROTECTED,
      SAI_MACSEC_SA_STAT_IN_PKTS_UNCHECKED,
      SAI_MACSEC_SA_STAT_IN_PKTS_DELAYED,
      SAI_MACSEC_SA_STAT_IN_PKTS_LATE,
      SAI_MACSEC_SA_STAT_IN_PKTS_INVALID,
      SAI_MACSEC_SA_STAT_IN_PKTS_NOT_VALID,
      SAI_MACSEC_SA_STAT_IN_PKTS_NOT_USING_SA,
      SAI_MACSEC_SA_STAT_IN_PKTS_UNUSED_SA,
      SAI_MACSEC_SA_STAT_IN_PKTS_OK,
  };

  static constexpr std::array<sai_stat_id_t, 0> CounterIdsToReadAndClear = {};
};

SAI_ATTRIBUTE_NAME(MacsecSA, AssocNum)
SAI_ATTRIBUTE_NAME(MacsecSA, AuthKey)
SAI_ATTRIBUTE_NAME(MacsecSA, MacsecDirection)
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
SAI_ATTRIBUTE_NAME(MacsecSA, SSCI)
#endif
SAI_ATTRIBUTE_NAME(MacsecSA, MinimumXpn)
SAI_ATTRIBUTE_NAME(MacsecSA, SAK)
SAI_ATTRIBUTE_NAME(MacsecSA, Salt)
SAI_ATTRIBUTE_NAME(MacsecSA, SCID)
#if SAI_API_VERSION >= SAI_VERSION(1, 8, 1)
SAI_ATTRIBUTE_NAME(MacsecSA, CurrentXpn)
#endif

template <>
struct SaiObjectHasStats<SaiMacsecSATraits> : public std::true_type {};

struct SaiMacsecSCTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_MACSEC_SC;
  using SaiApiT = MacsecApi;
  struct Attributes {
    using EnumType = sai_macsec_sc_attr_t;
    using SCI =
        SaiAttribute<EnumType, SAI_MACSEC_SC_ATTR_MACSEC_SCI, sai_uint64_t>;
    using MacsecDirection = SaiAttribute<
        EnumType,
        SAI_MACSEC_SC_ATTR_MACSEC_DIRECTION,
        sai_int32_t>;
    using ActiveEgressSAID = SaiAttribute<
        EnumType,
        SAI_MACSEC_SC_ATTR_ACTIVE_EGRESS_SA_ID,
        sai_object_id_t>;
    using FlowID =
        SaiAttribute<EnumType, SAI_MACSEC_SC_ATTR_FLOW_ID, sai_object_id_t>;
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
    using CipherSuite = SaiAttribute<
        EnumType,
        SAI_MACSEC_SC_ATTR_MACSEC_CIPHER_SUITE,
        sai_int32_t>;
#endif
    using SCIEnable = SaiAttribute<
        EnumType,
        SAI_MACSEC_SC_ATTR_MACSEC_EXPLICIT_SCI_ENABLE,
        bool>;
    using ReplayProtectionEnable = SaiAttribute<
        EnumType,
        SAI_MACSEC_SC_ATTR_MACSEC_REPLAY_PROTECTION_ENABLE,
        bool>;
    using ReplayProtectionWindow = SaiAttribute<
        EnumType,
        SAI_MACSEC_SC_ATTR_MACSEC_REPLAY_PROTECTION_WINDOW,
        sai_int32_t>;
    using SectagOffset = SaiAttribute<
        EnumType,
        SAI_MACSEC_SC_ATTR_MACSEC_SECTAG_OFFSET,
        sai_uint8_t>;
  };

  using AdapterKey = MacsecSCSaiId;
  using AdapterHostKey =
      std::tuple<Attributes::SCI, Attributes::MacsecDirection>;
  using CreateAttributes = std::tuple<
      Attributes::SCI,
      Attributes::MacsecDirection,
      Attributes::FlowID,
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
      Attributes::CipherSuite,
#endif
      std::optional<Attributes::SCIEnable>,
      std::optional<Attributes::ReplayProtectionEnable>,
      std::optional<Attributes::ReplayProtectionWindow>,
      std::optional<Attributes::SectagOffset>>;
};

SAI_ATTRIBUTE_NAME(MacsecSC, SCI)
SAI_ATTRIBUTE_NAME(MacsecSC, MacsecDirection)
SAI_ATTRIBUTE_NAME(MacsecSC, ActiveEgressSAID)
SAI_ATTRIBUTE_NAME(MacsecSC, FlowID)
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
SAI_ATTRIBUTE_NAME(MacsecSC, CipherSuite)
#endif
SAI_ATTRIBUTE_NAME(MacsecSC, SCIEnable)
SAI_ATTRIBUTE_NAME(MacsecSC, ReplayProtectionEnable)
SAI_ATTRIBUTE_NAME(MacsecSC, ReplayProtectionWindow)
SAI_ATTRIBUTE_NAME(MacsecSC, SectagOffset)

struct SaiMacsecFlowTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_MACSEC_FLOW;
  using SaiApiT = MacsecApi;
  struct Attributes {
    using EnumType = sai_macsec_flow_attr_t;
    using MacsecDirection = SaiAttribute<
        EnumType,
        SAI_MACSEC_FLOW_ATTR_MACSEC_DIRECTION,
        sai_int32_t>;
  };
  using AdapterKey = MacsecFlowSaiId;
  using AdapterHostKey = Attributes::MacsecDirection;
  using CreateAttributes = std::tuple<Attributes::MacsecDirection>;

  static constexpr std::array<sai_stat_id_t, 19> CounterIdsToRead = {
      SAI_MACSEC_FLOW_STAT_UCAST_PKTS_UNCONTROLLED,
      SAI_MACSEC_FLOW_STAT_UCAST_PKTS_CONTROLLED,
      SAI_MACSEC_FLOW_STAT_MULTICAST_PKTS_UNCONTROLLED,
      SAI_MACSEC_FLOW_STAT_MULTICAST_PKTS_CONTROLLED,
      SAI_MACSEC_FLOW_STAT_BROADCAST_PKTS_UNCONTROLLED,
      SAI_MACSEC_FLOW_STAT_BROADCAST_PKTS_CONTROLLED,
      SAI_MACSEC_FLOW_STAT_CONTROL_PKTS,
      SAI_MACSEC_FLOW_STAT_PKTS_UNTAGGED,
      SAI_MACSEC_FLOW_STAT_OTHER_ERR,
      SAI_MACSEC_FLOW_STAT_OCTETS_UNCONTROLLED,
      SAI_MACSEC_FLOW_STAT_OCTETS_CONTROLLED,
      SAI_MACSEC_FLOW_STAT_OUT_OCTETS_COMMON,
      SAI_MACSEC_FLOW_STAT_OUT_PKTS_TOO_LONG,
      SAI_MACSEC_FLOW_STAT_IN_TAGGED_CONTROL_PKTS,
      SAI_MACSEC_FLOW_STAT_IN_PKTS_NO_TAG,
      SAI_MACSEC_FLOW_STAT_IN_PKTS_BAD_TAG,
      SAI_MACSEC_FLOW_STAT_IN_PKTS_NO_SCI,
      SAI_MACSEC_FLOW_STAT_IN_PKTS_UNKNOWN_SCI,
      SAI_MACSEC_FLOW_STAT_IN_PKTS_OVERRUN,
  };
  static constexpr std::array<sai_stat_id_t, 0> CounterIdsToReadAndClear = {};
};

SAI_ATTRIBUTE_NAME(MacsecFlow, MacsecDirection)

template <>
struct SaiObjectHasStats<SaiMacsecFlowTraits> : public std::true_type {};

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

  sai_status_t _create(
      MacsecSASaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_macsec_sa(rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _create(
      MacsecSCSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_macsec_sc(rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _create(
      MacsecFlowSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_macsec_flow(rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(MacsecSaiId key) const {
    return api_->remove_macsec(key);
  }

  sai_status_t _remove(MacsecPortSaiId key) const {
    return api_->remove_macsec_port(key);
  }

  sai_status_t _remove(MacsecSASaiId key) const {
    return api_->remove_macsec_sa(key);
  }

  sai_status_t _remove(MacsecSCSaiId key) const {
    return api_->remove_macsec_sc(key);
  }

  sai_status_t _remove(MacsecFlowSaiId key) const {
    return api_->remove_macsec_flow(key);
  }

  sai_status_t _getAttribute(MacsecSaiId key, sai_attribute_t* attr) const {
    return api_->get_macsec_attribute(key, 1, attr);
  }

  sai_status_t _getAttribute(MacsecPortSaiId key, sai_attribute_t* attr) const {
    return api_->get_macsec_port_attribute(key, 1, attr);
  }

  sai_status_t _getAttribute(MacsecSASaiId key, sai_attribute_t* attr) const {
    return api_->get_macsec_sa_attribute(key, 1, attr);
  }

  sai_status_t _getAttribute(MacsecSCSaiId key, sai_attribute_t* attr) const {
    return api_->get_macsec_sc_attribute(key, 1, attr);
  }

  sai_status_t _getAttribute(MacsecFlowSaiId key, sai_attribute_t* attr) const {
    return api_->get_macsec_flow_attribute(key, 1, attr);
  }

  sai_status_t _setAttribute(MacsecSaiId key, const sai_attribute_t* attr)
      const {
    return api_->set_macsec_attribute(key, attr);
  }

  sai_status_t _setAttribute(MacsecPortSaiId key, const sai_attribute_t* attr)
      const {
    return api_->set_macsec_port_attribute(key, attr);
  }

  sai_status_t _setAttribute(MacsecSASaiId key, const sai_attribute_t* attr)
      const {
    return api_->set_macsec_sa_attribute(key, attr);
  }

  sai_status_t _setAttribute(MacsecSCSaiId key, const sai_attribute_t* attr)
      const {
    return api_->set_macsec_sc_attribute(key, attr);
  }

  sai_status_t _setAttribute(MacsecFlowSaiId key, const sai_attribute_t* attr)
      const {
    return api_->set_macsec_flow_attribute(key, attr);
  }

  sai_status_t _getStats(
      MacsecSASaiId key,
      uint32_t num_of_counters,
      const sai_stat_id_t* counter_ids,
      sai_stats_mode_t mode,
      uint64_t* counters) const {
    return api_->get_macsec_sa_stats(
        key, num_of_counters, counter_ids, counters);
  }

  sai_status_t _getStats(
      MacsecFlowSaiId key,
      uint32_t num_of_counters,
      const sai_stat_id_t* counter_ids,
      sai_stats_mode_t mode,
      uint64_t* counters) const {
    return api_->get_macsec_flow_stats(
        key, num_of_counters, counter_ids, counters);
  }

  sai_status_t _getStats(
      MacsecPortSaiId key,
      uint32_t num_of_counters,
      const sai_stat_id_t* counter_ids,
      sai_stats_mode_t mode,
      uint64_t* counters) const {
    return api_->get_macsec_port_stats(
        key, num_of_counters, counter_ids, counters);
  }

  sai_status_t _clearStats(
      MacsecSASaiId key,
      uint32_t num_of_counters,
      const sai_stat_id_t* counter_ids) const {
    return api_->clear_macsec_sa_stats(key, num_of_counters, counter_ids);
  }

  sai_status_t _clearStats(
      MacsecFlowSaiId key,
      uint32_t num_of_counters,
      const sai_stat_id_t* counter_ids) const {
    return api_->clear_macsec_flow_stats(key, num_of_counters, counter_ids);
  }

  sai_macsec_api_t* api_;
  friend class SaiApi<MacsecApi>;
};

} // namespace facebook::fboss
