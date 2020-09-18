/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "FakeSaiPort.h"
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>
#include <optional>

using facebook::fboss::FakePort;
using facebook::fboss::FakeSai;

sai_status_t create_port_fn(
    sai_object_id_t* port_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  std::optional<bool> adminState;
  std::vector<uint32_t> lanes;
  std::optional<sai_uint32_t> speed;
  std::optional<sai_port_fec_mode_t> fecMode;
  std::optional<sai_port_internal_loopback_mode_t> internalLoopbackMode;
  std::optional<sai_port_flow_control_mode_t> flowControlMode;
  std::optional<sai_port_media_type_t> mediaType;
  std::optional<sai_vlan_id_t> vlanId;
  std::vector<uint32_t> preemphasis;
  sai_uint32_t mtu{1514};
  sai_object_id_t qosDscpToTcMap{SAI_NULL_OBJECT_ID};
  sai_object_id_t qosTcToQueueMap{SAI_NULL_OBJECT_ID};
  bool disableTtlDecrement{false};
  sai_port_interface_type_t interface_type{SAI_PORT_INTERFACE_TYPE_NONE};
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_PORT_ATTR_ADMIN_STATE:
        adminState = attr_list[i].value.booldata;
        break;
      case SAI_PORT_ATTR_HW_LANE_LIST: {
        for (int j = 0; j < attr_list[i].value.u32list.count; ++j) {
          lanes.push_back(attr_list[i].value.u32list.list[j]);
        }
      } break;
      case SAI_PORT_ATTR_SPEED:
        speed = attr_list[i].value.u32;
        break;
      case SAI_PORT_ATTR_FEC_MODE:
        fecMode = static_cast<sai_port_fec_mode_t>(attr_list[i].value.u32);
        break;
      case SAI_PORT_ATTR_INTERNAL_LOOPBACK_MODE:
        internalLoopbackMode = static_cast<sai_port_internal_loopback_mode_t>(
            attr_list[i].value.u32);
        break;
      case SAI_PORT_ATTR_MEDIA_TYPE:
        mediaType = static_cast<sai_port_media_type_t>(attr_list[i].value.s32);
        break;
      case SAI_PORT_ATTR_GLOBAL_FLOW_CONTROL_MODE:
        flowControlMode =
            static_cast<sai_port_flow_control_mode_t>(attr_list[i].value.s32);
        break;
      case SAI_PORT_ATTR_PORT_VLAN_ID:
        vlanId = attr_list[i].value.u16;
        break;
      case SAI_PORT_ATTR_SERDES_PREEMPHASIS: {
        for (int j = 0; j < attr_list[i].value.u32list.count; ++j) {
          preemphasis.push_back(attr_list[i].value.u32list.list[j]);
        }
      } break;
      case SAI_PORT_ATTR_MTU:
        mtu = attr_list[i].value.u32;
        break;
      case SAI_PORT_ATTR_QOS_DSCP_TO_TC_MAP:
        qosDscpToTcMap = attr_list[i].value.oid;
        break;
      case SAI_PORT_ATTR_QOS_TC_TO_QUEUE_MAP:
        qosTcToQueueMap = attr_list[i].value.oid;
        break;
      case SAI_PORT_ATTR_DECREMENT_TTL:
        disableTtlDecrement = attr_list[i].value.booldata;
        break;
      case SAI_PORT_ATTR_INTERFACE_TYPE:
        interface_type =
            static_cast<sai_port_interface_type_t>(attr_list[i].value.s32);
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  if (lanes.empty() || !speed) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  *port_id = fs->portManager.create(lanes, speed.value());
  auto& port = fs->portManager.get(*port_id);
  if (adminState) {
    port.adminState = adminState.value();
  }
  if (fecMode.has_value()) {
    port.fecMode = fecMode.value();
  }
  if (internalLoopbackMode.has_value()) {
    port.internalLoopbackMode = internalLoopbackMode.value();
  }
  if (vlanId.has_value()) {
    port.vlanId = vlanId.value();
  }
  if (mediaType.has_value()) {
    port.mediaType = mediaType.value();
  }
  if (vlanId.has_value()) {
    port.vlanId = vlanId.value();
  }
  if (preemphasis.size()) {
    port.preemphasis = preemphasis;
  }
  port.mtu = mtu;
  port.qosDscpToTcMap = qosDscpToTcMap;
  port.qosTcToQueueMap = qosTcToQueueMap;
  port.disableTtlDecrement = disableTtlDecrement;
  // TODO: Use number of queues by querying SAI_SWITCH_ATTR_NUMBER_OF_QUEUES
  for (uint8_t queueId = 0; queueId < 7; queueId++) {
    auto saiQueueId = fs->queueManager.create(
        SAI_QUEUE_TYPE_UNICAST, *port_id, queueId, *port_id);
    port.queueIdList.push_back(saiQueueId);
    if (queueId == 6) {
      // Create queue 6 for multicast also.
      saiQueueId = fs->queueManager.create(
          SAI_QUEUE_TYPE_MULTICAST, *port_id, queueId, *port_id);
      port.queueIdList.push_back(saiQueueId);
    }
  }
  port.interface_type = interface_type;
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_port_fn(sai_object_id_t port_id) {
  if (FakeSai::getInstance()->getCpuPort() == port_id) {
    // ignore removing CPU port
    return SAI_STATUS_SUCCESS;
  }
  auto fs = FakeSai::getInstance();
  auto& port = fs->portManager.get(port_id);
  for (auto saiQueueId : port.queueIdList) {
    fs->queueManager.remove(saiQueueId);
  }
  fs->portManager.remove(port_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_port_attribute_fn(
    sai_object_id_t port_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& port = fs->portManager.get(port_id);
  sai_status_t res = SAI_STATUS_SUCCESS;
  if (!attr) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  switch (attr->id) {
    case SAI_PORT_ATTR_ADMIN_STATE:
      port.adminState = attr->value.booldata;
      break;
    case SAI_PORT_ATTR_HW_LANE_LIST: {
      auto& lanes = port.lanes;
      lanes.clear();
      for (int j = 0; j < attr->value.u32list.count; ++j) {
        lanes.push_back(attr->value.u32list.list[j]);
      }
    } break;
    case SAI_PORT_ATTR_SPEED:
      port.speed = attr->value.u32;
      break;
    case SAI_PORT_ATTR_FEC_MODE:
      port.fecMode = static_cast<sai_port_fec_mode_t>(attr->value.s32);
      break;
    case SAI_PORT_ATTR_INTERNAL_LOOPBACK_MODE:
      port.internalLoopbackMode =
          static_cast<sai_port_internal_loopback_mode_t>(attr->value.s32);
      break;
    case SAI_PORT_ATTR_MEDIA_TYPE:
      port.mediaType = static_cast<sai_port_media_type_t>(attr->value.s32);
      break;
    case SAI_PORT_ATTR_GLOBAL_FLOW_CONTROL_MODE:
      port.globalFlowControlMode =
          static_cast<sai_port_flow_control_mode_t>(attr->value.s32);
      break;
    case SAI_PORT_ATTR_PORT_VLAN_ID:
      port.vlanId = static_cast<sai_vlan_id_t>(attr->value.u16);
      break;
    case SAI_PORT_ATTR_SERDES_PREEMPHASIS: {
      auto& preemphasis = port.preemphasis;
      preemphasis.clear();
      for (int j = 0; j < attr->value.u32list.count; ++j) {
        preemphasis.push_back(attr->value.u32list.list[j]);
      }
    } break;
    case SAI_PORT_ATTR_MTU:
      port.mtu = attr->value.u32;
      break;
    case SAI_PORT_ATTR_QOS_DSCP_TO_TC_MAP:
      port.qosDscpToTcMap = attr->value.oid;
      break;
    case SAI_PORT_ATTR_QOS_TC_TO_QUEUE_MAP:
      port.qosTcToQueueMap = attr->value.oid;
      break;
    case SAI_PORT_ATTR_DECREMENT_TTL:
      port.disableTtlDecrement = attr->value.booldata;
      break;
    case SAI_PORT_ATTR_INTERFACE_TYPE:
      port.interface_type =
          static_cast<sai_port_interface_type_t>(attr->value.s32);
      break;
    default:
      res = SAI_STATUS_INVALID_PARAMETER;
      break;
  }
  return res;
}

sai_status_t get_port_attribute_fn(
    sai_object_id_t port_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  const auto& port = fs->portManager.get(port_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_PORT_ATTR_ADMIN_STATE:
        attr[i].value.booldata = port.adminState;
        break;
      case SAI_PORT_ATTR_HW_LANE_LIST:
        if (port.lanes.size() > attr[i].value.u32list.count) {
          attr[i].value.u32list.count = port.lanes.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        for (int j = 0; j < port.lanes.size(); ++j) {
          attr[i].value.u32list.list[j] = port.lanes[j];
        }
        attr[i].value.u32list.count = port.lanes.size();
        break;
      case SAI_PORT_ATTR_SPEED:
        attr[i].value.u32 = port.speed;
        break;
      case SAI_PORT_ATTR_QOS_NUMBER_OF_QUEUES:
        attr[i].value.u32 = 8;
        break;
      case SAI_PORT_ATTR_QOS_QUEUE_LIST:
        if (port.queueIdList.size() > attr[i].value.objlist.count) {
          attr[i].value.objlist.count = port.queueIdList.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        for (int j = 0; j < port.queueIdList.size(); ++j) {
          attr[i].value.objlist.list[j] = port.queueIdList[j];
        }
        break;
      case SAI_PORT_ATTR_FEC_MODE:
        attr[i].value.s32 = static_cast<int32_t>(port.fecMode);
        break;
      case SAI_PORT_ATTR_INTERNAL_LOOPBACK_MODE:
        attr[i].value.s32 = static_cast<int32_t>(port.internalLoopbackMode);
        break;
      case SAI_PORT_ATTR_MEDIA_TYPE:
        attr[i].value.s32 = static_cast<int32_t>(port.mediaType);
        break;
      case SAI_PORT_ATTR_GLOBAL_FLOW_CONTROL_MODE:
        attr[i].value.s32 = static_cast<int32_t>(port.globalFlowControlMode);
        break;
      case SAI_PORT_ATTR_PORT_VLAN_ID:
        attr[i].value.u16 = port.vlanId;
        break;
      case SAI_PORT_ATTR_SERDES_PREEMPHASIS:
        if (port.preemphasis.size() > attr[i].value.u32list.count) {
          attr[i].value.u32list.count = port.preemphasis.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        for (int j = 0; j < port.preemphasis.size(); ++j) {
          attr[i].value.u32list.list[j] = port.preemphasis[j];
        }
        attr[i].value.u32list.count = port.preemphasis.size();
        break;
      case SAI_PORT_ATTR_MTU:
        attr->value.u32 = port.mtu;
        break;
      case SAI_PORT_ATTR_OPER_STATUS:
        attr->value.s32 = SAI_PORT_OPER_STATUS_UP;
        break;
      case SAI_PORT_ATTR_QOS_DSCP_TO_TC_MAP:
        attr->value.oid = port.qosDscpToTcMap;
        break;
      case SAI_PORT_ATTR_QOS_TC_TO_QUEUE_MAP:
        attr->value.oid = port.qosTcToQueueMap;
        break;
      case SAI_PORT_ATTR_DECREMENT_TTL:
        attr->value.booldata = port.disableTtlDecrement;
        break;
      case SAI_PORT_ATTR_INTERFACE_TYPE:
        attr->value.s32 = port.interface_type;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_port_stats_fn(
    sai_object_id_t /*port*/,
    uint32_t num_of_counters,
    const sai_stat_id_t* /*counter_ids*/,
    uint64_t* counters) {
  for (auto i = 0; i < num_of_counters; ++i) {
    counters[i] = 0;
  }
  return SAI_STATUS_SUCCESS;
}

/*
 * In fake sai there isn't a dataplane, so all stats
 * stay at 0. Leverage the corresponding non _ext
 * stats fn to get the stats. If stats are always 0,
 * modes (READ, READ_AND_CLEAR) don't matter
 */
sai_status_t get_port_stats_ext_fn(
    sai_object_id_t port,
    uint32_t num_of_counters,
    const sai_stat_id_t* counter_ids,
    sai_stats_mode_t /*mode*/,
    uint64_t* counters) {
  return get_port_stats_fn(port, num_of_counters, counter_ids, counters);
}
/*
 *  noop clear stats API. Since fake doesnt have a
 *  dataplane stats are always set to 0, so
 *  no need to clear them
 */
sai_status_t clear_port_stats_fn(
    sai_object_id_t port_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids) {
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_port_serdes_attribute_fn(
    sai_object_id_t port_serdes_id,
    const sai_attribute_t* attr);

sai_status_t create_port_serdes_fn(
    sai_object_id_t* port_serdes_id,
    sai_object_id_t /*switch_id*/,
    uint32_t count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  bool created = false;
  for (auto i = 0; i < count; i++) {
    if (attr_list[i].id == SAI_PORT_SERDES_ATTR_PORT_ID) {
      created = true;
      *port_serdes_id = fs->portSerdesManager.create(attr_list[i].value.oid);
      break;
    }
  }
  if (!created) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  for (auto i = 0; i < count; i++) {
    if (attr_list[i].id == SAI_PORT_SERDES_ATTR_PORT_ID) {
      continue;
    }
    auto status = set_port_serdes_attribute_fn(*port_serdes_id, &attr_list[i]);
    if (status != SAI_STATUS_SUCCESS) {
      return status;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_port_serdes_fn(sai_object_id_t port_serdes_id) {
  auto fs = FakeSai::getInstance();
  fs->portSerdesManager.remove(port_serdes_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_port_serdes_attribute_fn(
    sai_object_id_t port_serdes_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& portSerdes = fs->portSerdesManager.get(port_serdes_id);
  auto& port = fs->portManager.get(portSerdes.port);
  auto fillVec = [](auto& vec, auto* list, size_t count) {
    std::copy(list, list + count, std::back_inserter(vec));
  };
  auto checkLanes = [&port](auto vec) {
    return port.lanes.size() == vec.size();
  };

  switch (attr->id) {
    case SAI_PORT_SERDES_ATTR_TX_FIR_PRE1:
      fillVec(
          portSerdes.txFirPre1,
          attr->value.u32list.list,
          attr->value.u32list.count);
      if (!checkLanes(portSerdes.txFirPre1)) {
        return SAI_STATUS_INVALID_ATTRIBUTE_0;
      }
      break;

    case SAI_PORT_SERDES_ATTR_TX_FIR_PRE2:
      fillVec(
          portSerdes.txFirPre2,
          attr->value.u32list.list,
          attr->value.u32list.count);
      if (!checkLanes(portSerdes.txFirPre2)) {
        return SAI_STATUS_INVALID_ATTRIBUTE_0;
      }
      break;

    case SAI_PORT_SERDES_ATTR_IDRIVER:
      fillVec(
          portSerdes.iDriver,
          attr->value.u32list.list,
          attr->value.u32list.count);
      if (!checkLanes(portSerdes.iDriver)) {
        return SAI_STATUS_INVALID_ATTRIBUTE_0;
      }
      break;

    case SAI_PORT_SERDES_ATTR_TX_FIR_MAIN:
      fillVec(
          portSerdes.txFirMain,
          attr->value.u32list.list,
          attr->value.u32list.count);
      if (!checkLanes(portSerdes.txFirMain)) {
        return SAI_STATUS_INVALID_ATTRIBUTE_0;
      }
      break;

    case SAI_PORT_SERDES_ATTR_TX_FIR_POST1:
      fillVec(
          portSerdes.txFirPost1,
          attr->value.u32list.list,
          attr->value.u32list.count);
      if (!checkLanes(portSerdes.txFirPost1)) {
        return SAI_STATUS_INVALID_ATTRIBUTE_0;
      }
      break;

    case SAI_PORT_SERDES_ATTR_TX_FIR_POST2:
      fillVec(
          portSerdes.txFirPost2,
          attr->value.u32list.list,
          attr->value.u32list.count);
      if (!checkLanes(portSerdes.txFirPost2)) {
        return SAI_STATUS_INVALID_ATTRIBUTE_0;
      }
      break;

    case SAI_PORT_SERDES_ATTR_TX_FIR_POST3:
      fillVec(
          portSerdes.txFirPost3,
          attr->value.u32list.list,
          attr->value.u32list.count);
      if (!checkLanes(portSerdes.txFirPost3)) {
        return SAI_STATUS_INVALID_ATTRIBUTE_0;
      }
      break;

    case SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_CTLE_CODE:
      fillVec(
          portSerdes.rxCtlCode,
          attr->value.s32list.list,
          attr->value.u32list.count);
      if (!checkLanes(portSerdes.rxCtlCode)) {
        return SAI_STATUS_INVALID_ATTRIBUTE_0;
      }
      break;

    case SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_DSP_MODE:
      fillVec(
          portSerdes.rxDspMode,
          attr->value.s32list.list,
          attr->value.s32list.count);
      if (!checkLanes(portSerdes.rxDspMode)) {
        return SAI_STATUS_INVALID_ATTRIBUTE_0;
      }
      break;

    case SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_AFE_TRIM:
      fillVec(
          portSerdes.rxAfeTrim,
          attr->value.s32list.list,
          attr->value.s32list.count);
      if (!checkLanes(portSerdes.rxAfeTrim)) {
        return SAI_STATUS_INVALID_ATTRIBUTE_0;
      }
      break;

    case SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_AC_COUPLING_BYPASS:
      fillVec(
          portSerdes.rxCouplingByPass,
          attr->value.s32list.list,
          attr->value.s32list.count);
      if (!checkLanes(portSerdes.rxCouplingByPass)) {
        return SAI_STATUS_INVALID_ATTRIBUTE_0;
      }
      break;

    default:
      return SAI_STATUS_NOT_SUPPORTED;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_port_serdes_attribute_fn(
    sai_object_id_t port_serdes_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  auto& portSerdes = fs->portSerdesManager.get(port_serdes_id);
  auto checkListSize = [](auto& list, auto& vec) {
    if (list.count < vec.size()) {
      return false;
    }
    return true;
  };
  auto copyVecToList = [](auto& vec, auto& list) {
    for (auto i = 0; i < vec.size(); i++) {
      list.list[i] = vec[i];
    }
  };
  for (auto i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_PORT_SERDES_ATTR_PORT_ID:
        attr_list[i].value.oid = portSerdes.port;
        break;
      case SAI_PORT_SERDES_ATTR_IDRIVER:
        if (!checkListSize(attr_list[i].value.u32list, portSerdes.iDriver)) {
          attr_list[i].value.u32list.count = portSerdes.iDriver.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        copyVecToList(portSerdes.iDriver, attr_list[i].value.u32list);
        break;
      case SAI_PORT_SERDES_ATTR_TX_FIR_PRE1:
        if (!checkListSize(attr_list[i].value.u32list, portSerdes.txFirPre1)) {
          attr_list[i].value.u32list.count = portSerdes.txFirPre1.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        copyVecToList(portSerdes.txFirPre1, attr_list[i].value.u32list);
        break;
      case SAI_PORT_SERDES_ATTR_TX_FIR_PRE2:
        if (!checkListSize(attr_list[i].value.u32list, portSerdes.txFirPre2)) {
          attr_list[i].value.u32list.count = portSerdes.txFirPre2.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        copyVecToList(portSerdes.txFirPre2, attr_list[i].value.u32list);
        break;
      case SAI_PORT_SERDES_ATTR_TX_FIR_MAIN:
        if (!checkListSize(attr_list[i].value.u32list, portSerdes.txFirMain)) {
          attr_list[i].value.u32list.count = portSerdes.txFirMain.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        copyVecToList(portSerdes.txFirMain, attr_list[i].value.u32list);
        break;
      case SAI_PORT_SERDES_ATTR_TX_FIR_POST1:
        if (!checkListSize(attr_list[i].value.u32list, portSerdes.txFirPost1)) {
          attr_list[i].value.u32list.count = portSerdes.txFirPost1.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        copyVecToList(portSerdes.txFirPost1, attr_list[i].value.u32list);
        break;
      case SAI_PORT_SERDES_ATTR_TX_FIR_POST2:
        if (!checkListSize(attr_list[i].value.u32list, portSerdes.txFirPost2)) {
          attr_list[i].value.u32list.count = portSerdes.txFirPost2.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        copyVecToList(portSerdes.txFirPost2, attr_list[i].value.u32list);
        break;
      case SAI_PORT_SERDES_ATTR_TX_FIR_POST3:
        if (!checkListSize(attr_list[i].value.u32list, portSerdes.txFirPost3)) {
          attr_list[i].value.u32list.count = portSerdes.txFirPost3.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        copyVecToList(portSerdes.txFirPost3, attr_list[i].value.u32list);
        break;
      case SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_CTLE_CODE:
        if (!checkListSize(attr_list[i].value.s32list, portSerdes.rxCtlCode)) {
          attr_list[i].value.s32list.count = portSerdes.rxCtlCode.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        copyVecToList(portSerdes.rxCtlCode, attr_list[i].value.s32list);
        break;
      case SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_DSP_MODE:
        if (!checkListSize(attr_list[i].value.s32list, portSerdes.rxDspMode)) {
          attr_list[i].value.s32list.count = portSerdes.rxDspMode.size();

          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        copyVecToList(portSerdes.rxDspMode, attr_list[i].value.s32list);
        break;
      case SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_AFE_TRIM:
        if (!checkListSize(attr_list[i].value.s32list, portSerdes.rxAfeTrim)) {
          attr_list[i].value.s32list.count = portSerdes.rxAfeTrim.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        copyVecToList(portSerdes.rxAfeTrim, attr_list[i].value.s32list);
        break;
      case SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_AC_COUPLING_BYPASS:
        if (!checkListSize(
                attr_list[i].value.s32list, portSerdes.rxCouplingByPass)) {
          attr_list[i].value.s32list.count = portSerdes.rxCouplingByPass.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        copyVecToList(portSerdes.rxCouplingByPass, attr_list[i].value.s32list);
        break;
      default:
        return SAI_STATUS_NOT_IMPLEMENTED;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

static sai_port_api_t _port_api;

void populate_port_api(sai_port_api_t** port_api) {
  _port_api.create_port = &create_port_fn;
  _port_api.remove_port = &remove_port_fn;
  _port_api.set_port_attribute = &set_port_attribute_fn;
  _port_api.get_port_attribute = &get_port_attribute_fn;
  _port_api.get_port_stats = &get_port_stats_fn;
  _port_api.get_port_stats_ext = &get_port_stats_ext_fn;
  _port_api.clear_port_stats = &clear_port_stats_fn;
  _port_api.create_port_serdes = &create_port_serdes_fn;
  _port_api.remove_port_serdes = &remove_port_serdes_fn;
  _port_api.set_port_serdes_attribute = &set_port_serdes_attribute_fn;
  _port_api.get_port_serdes_attribute = &get_port_serdes_attribute_fn;
  *port_api = &_port_api;
}

} // namespace facebook::fboss
