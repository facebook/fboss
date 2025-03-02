/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/LoggingUtil.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/sai/api/SaiVersion.h"

#include <folly/Format.h>

extern "C" {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
#include <saiextensions.h>
#ifndef IS_OSS_BRCM_SAI
#include <experimental/saiexperimentaltameventaginggroup.h>
#else
#include <saiexperimentaltameventaginggroup.h>
#endif
#endif
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
#ifndef IS_OSS_BRCM_SAI
#include <experimental/saiexperimentalvendorswitch.h>
#else
#include <saiexperimentalvendorswitch.h>
#endif
#endif
}

namespace facebook::fboss {

folly::StringPiece saiApiTypeToString(sai_api_t apiType) {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  if (UNLIKELY(apiType >= SAI_API_MAX)) {
    switch (static_cast<sai_api_extensions_t>(apiType)) {
      case SAI_API_TAM_EVENT_AGING_GROUP:
        return "tam-event-aging-group";
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
      case SAI_API_VENDOR_SWITCH:
        return "vendor-switch";
#endif
      default:
        break;
    }
  }
#endif

  switch (apiType) {
    case SAI_API_UNSPECIFIED:
      return "unspecified";
    case SAI_API_SWITCH:
      return "switch";
    case SAI_API_PORT:
      return "port";
    case SAI_API_FDB:
      return "fdb";
    case SAI_API_VLAN:
      return "vlan";
    case SAI_API_VIRTUAL_ROUTER:
      return "vr";
    case SAI_API_ROUTE:
      return "route";
    case SAI_API_NEXT_HOP:
      return "nhop";
    case SAI_API_NEXT_HOP_GROUP:
      return "nhop-group";
    case SAI_API_ROUTER_INTERFACE:
      return "rif";
    case SAI_API_NEIGHBOR:
      return "neighbor";
    case SAI_API_ACL:
      return "acl";
    case SAI_API_HOSTIF:
      return "hostif";
    case SAI_API_MIRROR:
      return "mirror";
    case SAI_API_SAMPLEPACKET:
      return "samplepacket";
    case SAI_API_STP:
      return "stp";
    case SAI_API_LAG:
      return "lag";
    case SAI_API_POLICER:
      return "policer";
    case SAI_API_WRED:
      return "wred";
    case SAI_API_QOS_MAP:
      return "qos-map";
    case SAI_API_QUEUE:
      return "queue";
    case SAI_API_SCHEDULER:
      return "scheduler";
    case SAI_API_SCHEDULER_GROUP:
      return "scheduler-group";
    case SAI_API_BUFFER:
      return "buffer";
    case SAI_API_HASH:
      return "hash";
    case SAI_API_UDF:
      return "udf";
    case SAI_API_TUNNEL:
      return "tunnel";
    case SAI_API_L2MC:
      return "l2mc";
    case SAI_API_IPMC:
      return "ipmc";
    case SAI_API_RPF_GROUP:
      return "rpf_group";
    case SAI_API_L2MC_GROUP:
      return "l2mc_group";
    case SAI_API_IPMC_GROUP:
      return "ipmc_group";
    case SAI_API_MCAST_FDB:
      return "mcast_fdb";
    case SAI_API_BRIDGE:
      return "bridge";
    case SAI_API_TAM:
      return "tam";
#if SAI_API_VERSION < SAI_VERSION(1, 10, 0)
    case SAI_API_SEGMENTROUTE:
      return "segmentroute";
#endif
    case SAI_API_MPLS:
      return "mpls";
    case SAI_API_DTEL:
      return "dtel";
    case SAI_API_BFD:
      return "bfd";
    case SAI_API_ISOLATION_GROUP:
      return "isolation_group";
    case SAI_API_NAT:
      return "nat";
    case SAI_API_COUNTER:
      return "counter";
    case SAI_API_DEBUG_COUNTER:
      return "debug_counter";
    case SAI_API_MACSEC:
      return "macsec";
    case SAI_API_SYSTEM_PORT:
      return "system_port";
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
    case SAI_API_ARS:
      return "ars";
    case SAI_API_ARS_PROFILE:
      return "ars_profile";
#endif
    default:
      if (apiType >= SAI_API_MAX) {
        throw FbossError("api type invalid: ", apiType);
      }
      return "unsupported";
  }
}

folly::StringPiece saiObjectTypeToString(sai_object_type_t objectType) {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  if (UNLIKELY(objectType >= SAI_OBJECT_TYPE_MAX)) {
    switch (static_cast<sai_object_type_extensions_t>(objectType)) {
      case SAI_OBJECT_TYPE_TAM_EVENT_AGING_GROUP:
        return "tam-event-aging-group";
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
      case SAI_OBJECT_TYPE_VENDOR_SWITCH:
        return "vendor-switch";
#endif
      default:
        throw FbossError("object type extension invalid: ", objectType);
    }
  }
#endif

  switch (objectType) {
    case SAI_OBJECT_TYPE_NULL:
      return "null";
    case SAI_OBJECT_TYPE_PORT:
      return "port";
    case SAI_OBJECT_TYPE_LAG:
      return "lag";
    case SAI_OBJECT_TYPE_VIRTUAL_ROUTER:
      return "vr";
    case SAI_OBJECT_TYPE_NEXT_HOP:
      return "nhop";
    case SAI_OBJECT_TYPE_NEXT_HOP_GROUP:
      return "nhop-group";
    case SAI_OBJECT_TYPE_ROUTER_INTERFACE:
      return "rif";
    case SAI_OBJECT_TYPE_HOSTIF:
      return "hostif";
    case SAI_OBJECT_TYPE_HASH:
      return "hash";
    case SAI_OBJECT_TYPE_MIRROR_SESSION:
      return "mirror-session";
    case SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP:
      return "hostif-trap-group";
    case SAI_OBJECT_TYPE_QOS_MAP:
      return "qos-map";
    case SAI_OBJECT_TYPE_QUEUE:
      return "queue";
    case SAI_OBJECT_TYPE_SCHEDULER:
      return "scheduler";
    case SAI_OBJECT_TYPE_SCHEDULER_GROUP:
      return "scheduler-group";
    case SAI_OBJECT_TYPE_LAG_MEMBER:
      return "lag-member";
    case SAI_OBJECT_TYPE_FDB_ENTRY:
      return "fdb-entry";
    case SAI_OBJECT_TYPE_SWITCH:
      return "switch";
    case SAI_OBJECT_TYPE_HOSTIF_TRAP:
      return "hostif-trap";
    case SAI_OBJECT_TYPE_NEIGHBOR_ENTRY:
      return "neighbor-entry";
    case SAI_OBJECT_TYPE_ROUTE_ENTRY:
      return "route-entry";
    case SAI_OBJECT_TYPE_VLAN:
      return "vlan";
    case SAI_OBJECT_TYPE_VLAN_MEMBER:
      return "vlan-member";
    case SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER:
      return "nhop-group-member";
    case SAI_OBJECT_TYPE_BRIDGE:
      return "bridge";
    case SAI_OBJECT_TYPE_BRIDGE_PORT:
      return "bridge-port";
    case SAI_OBJECT_TYPE_BUFFER_POOL:
      return "buffer-pool";
    case SAI_OBJECT_TYPE_BUFFER_PROFILE:
      return "buffer-profile";
    case SAI_OBJECT_TYPE_INGRESS_PRIORITY_GROUP:
      return "ingress-priority-group";
    case SAI_OBJECT_TYPE_INSEG_ENTRY:
      return "inseg-entry";
    case SAI_OBJECT_TYPE_ACL_TABLE_GROUP:
      return "acl-table-group";
    case SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER:
      return "acl-table-group-member";
    case SAI_OBJECT_TYPE_ACL_TABLE:
      return "acl-table";
    case SAI_OBJECT_TYPE_ACL_ENTRY:
      return "acl-entry";
    case SAI_OBJECT_TYPE_ACL_COUNTER:
      return "acl-counter";
    case SAI_OBJECT_TYPE_PORT_SERDES:
      return "port-serdes";
    case SAI_OBJECT_TYPE_PORT_CONNECTOR:
      return "port-connector";
    case SAI_OBJECT_TYPE_COUNTER:
      return "counter";
    case SAI_OBJECT_TYPE_DEBUG_COUNTER:
      return "debug-counter";
    case SAI_OBJECT_TYPE_WRED:
      return "wred";
    case SAI_OBJECT_TYPE_TAM_COLLECTOR:
      return "tam-collector";
    case SAI_OBJECT_TYPE_TAM_TRANSPORT:
      return "tam-transport";
    case SAI_OBJECT_TYPE_TAM_REPORT:
      return "tam-report";
    case SAI_OBJECT_TYPE_TAM_EVENT_ACTION:
      return "tam-event-action";
    case SAI_OBJECT_TYPE_TAM_EVENT:
      return "tam-event";
    case SAI_OBJECT_TYPE_TAM:
      return "tam";
    case SAI_OBJECT_TYPE_TUNNEL:
      return "tunnel";
    case SAI_OBJECT_TYPE_TUNNEL_TERM_TABLE_ENTRY:
      return "tunnel-term";
    case SAI_OBJECT_TYPE_SAMPLEPACKET:
      return "sample-packet";
    case SAI_OBJECT_TYPE_MACSEC:
      return "macsec";
    case SAI_OBJECT_TYPE_MACSEC_PORT:
      return "macsec-port";
    case SAI_OBJECT_TYPE_MACSEC_SA:
      return "macsec-sa";
    case SAI_OBJECT_TYPE_MACSEC_SC:
      return "macsec-sc";
    case SAI_OBJECT_TYPE_MACSEC_FLOW:
      return "macsec-flow";
    case SAI_OBJECT_TYPE_SYSTEM_PORT:
      return "system-port";
    case SAI_OBJECT_TYPE_UDF:
      return "udf";
    case SAI_OBJECT_TYPE_UDF_GROUP:
      return "udf-group";
    case SAI_OBJECT_TYPE_UDF_MATCH:
      return "udf-match";
    case SAI_OBJECT_TYPE_HOSTIF_USER_DEFINED_TRAP:
      return "hostif-user-defined-trap";
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
    case SAI_OBJECT_TYPE_ARS:
      return "ars";
    case SAI_OBJECT_TYPE_ARS_PROFILE:
      return "ars_profile";
#endif
    default:
      throw FbossError("object type invalid: ", objectType);
  }
}

folly::StringPiece saiStatusToString(sai_status_t status) {
  switch (status) {
    case SAI_STATUS_SUCCESS:
      return "SUCCESS";
    case SAI_STATUS_FAILURE:
      return "FAILURE";
    case SAI_STATUS_NOT_SUPPORTED:
      return "NOT SUPPORTED";
    case SAI_STATUS_NO_MEMORY:
      return "NO MEMORY";
    case SAI_STATUS_INSUFFICIENT_RESOURCES:
      return "INSUFFICIENT RESOURCES";
    case SAI_STATUS_INVALID_PARAMETER:
      return "INVALID PARAMETER";
    case SAI_STATUS_ITEM_ALREADY_EXISTS:
      return "ITEM ALREADY EXISTS";
    case SAI_STATUS_ITEM_NOT_FOUND:
      return "ITEM NOT FOUND";
    case SAI_STATUS_BUFFER_OVERFLOW:
      return "BUFFER OVERFLOW";
    case SAI_STATUS_INVALID_PORT_NUMBER:
      return "INVALID PORT NUMBER";
    case SAI_STATUS_INVALID_PORT_MEMBER:
      return "INVALID PORT MEMBER";
    case SAI_STATUS_INVALID_VLAN_ID:
      return "INVALID VLAN ID";
    case SAI_STATUS_UNINITIALIZED:
      return "UNINITIALIZED";
    case SAI_STATUS_TABLE_FULL:
      return "TABLE FULL";
    case SAI_STATUS_MANDATORY_ATTRIBUTE_MISSING:
      return "MANDATORY ATTRIBUTE MISSING";
    case SAI_STATUS_NOT_IMPLEMENTED:
      return "NOT IMPLEMENTED";
    case SAI_STATUS_ADDR_NOT_FOUND:
      return "ADDR NOT FOUND";
    case SAI_STATUS_OBJECT_IN_USE:
      return "OBJECT IN USE";
    case SAI_STATUS_INVALID_OBJECT_TYPE:
      return "INVALID OBJECT TYPE";
    case SAI_STATUS_INVALID_OBJECT_ID:
      return "INVALID OBJECT ID";
    case SAI_STATUS_INVALID_NV_STORAGE:
      return "INVALID NV STORAGE";
    case SAI_STATUS_NV_STORAGE_FULL:
      return "NV STORAGE FULL";
    case SAI_STATUS_SW_UPGRADE_VERSION_MISMATCH:
      return "SW UPGRADE VERSION MISMATCH";
    case SAI_STATUS_NOT_EXECUTED:
      return "NOT EXECUTED";
    case SAI_STATUS_INVALID_ATTRIBUTE_0:
      return "INVALID ATTRIBUTE 0";
    case SAI_STATUS_INVALID_ATTRIBUTE_MAX:
      return "INVALID ATTRIBUTE MAX";
    case SAI_STATUS_INVALID_ATTR_VALUE_MAX:
      return "INVALID ATTR VALUE MAX";
    case SAI_STATUS_ATTR_NOT_IMPLEMENTED_0:
      return "ATTR NOT IMPLEMENTED 0";
    case SAI_STATUS_ATTR_NOT_IMPLEMENTED_MAX:
      return "ATTR NOT IMPLEMENTED MAX";
    case SAI_STATUS_UNKNOWN_ATTRIBUTE_0:
      return "UNKNOWN ATTRIBUTE 0";
    case SAI_STATUS_UNKNOWN_ATTRIBUTE_MAX:
      return "UNKNOWN ATTRIBUTE MAX";
    case SAI_STATUS_ATTR_NOT_SUPPORTED_0:
      return "ATTR NOT SUPPORTED 0";
    case SAI_STATUS_ATTR_NOT_SUPPORTED_MAX:
      return "ATTR NOT SUPPORTED MAX";
    default:
      if (SAI_STATUS_IS_INVALID_ATTRIBUTE(status)) {
        return "INVALID ATTRIBUTE";
      } else if (SAI_STATUS_IS_INVALID_ATTR_VALUE(status)) {
        return "INVALID ATTRIBUTE VALUE";
      } else if (SAI_STATUS_IS_ATTR_NOT_IMPLEMENTED(status)) {
        return "NOT IMPLEMENTED";
      } else if (SAI_STATUS_IS_UNKNOWN_ATTRIBUTE(status)) {
        return "UNKNOWN ATTRIBUTE";
      } else if (SAI_STATUS_IS_ATTR_NOT_SUPPORTED(status)) {
        return "NOT SUPPORTED";
      } else {
        throw FbossError("status invalid: ", status);
      }
  }
}

sai_log_level_t saiLogLevelFromString(const std::string& logLevel) {
  if (logLevel == "CRITICAL") {
    return SAI_LOG_LEVEL_CRITICAL;
  } else if (logLevel == "ERROR") {
    return SAI_LOG_LEVEL_ERROR;
  } else if (logLevel == "WARN") {
    return SAI_LOG_LEVEL_WARN;
  } else if (logLevel == "NOTICE") {
    return SAI_LOG_LEVEL_NOTICE;
  } else if (logLevel == "INFO") {
    return SAI_LOG_LEVEL_INFO;
  } else if (logLevel == "DEBUG") {
    return SAI_LOG_LEVEL_DEBUG;
  } else {
    throw FbossError("invalid log level set", logLevel);
  }
}

folly::StringPiece packetRxReasonToString(cfg::PacketRxReason rxReason) {
  switch (rxReason) {
    case cfg::PacketRxReason::ARP:
      return "arp-request";
    case cfg::PacketRxReason::ARP_RESPONSE:
      return "arp-response";
    case cfg::PacketRxReason::NDP:
      return "ipv6-ndp-discovery";
    case cfg::PacketRxReason::CPU_IS_NHOP:
      return "ip2me";
    case cfg::PacketRxReason::DHCP:
      return "dhcp";
    case cfg::PacketRxReason::LLDP:
      return "lldp";
    case cfg::PacketRxReason::BGP:
      return "bgp";
    case cfg::PacketRxReason::BGPV6:
      return "bgpv6";
    case cfg::PacketRxReason::LACP:
      return "lacp";
    case cfg::PacketRxReason::L3_MTU_ERROR:
      return "l3-mtu-error";
    case cfg::PacketRxReason::TTL_1:
      return "ttl-error";
    case cfg::PacketRxReason::MPLS_TTL_1:
      return "mpls-ttl-error";
    case cfg::PacketRxReason::TTL_0:
      return "ttl0-error";
    case cfg::PacketRxReason::DHCPV6:
      return "dhcpv6";
    case cfg::PacketRxReason::SAMPLEPACKET:
      return "samplepacket";
    case cfg::PacketRxReason::EAPOL:
      return "eapol";
    case cfg::PacketRxReason::PORT_MTU_ERROR:
      return "port-mtu-error";
    default:
      return "unknown-trap";
  }
}

} // namespace facebook::fboss
