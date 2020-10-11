/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "FakeSaiSwitch.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

using facebook::fboss::FakeSai;

namespace {
static constexpr uint64_t kDefaultVlanId = 0;
static constexpr uint64_t kDefaultVirtualRouterId = 0;
static constexpr uint64_t kDefault1QBridgeId = 0;
static constexpr uint64_t kCpuPort = 0;
static constexpr uint32_t kMaxPortUnicastQueues = 8;
static constexpr uint32_t kMaxPortMulticastQueues = 8;
static constexpr uint32_t kMaxPortQueues =
    kMaxPortUnicastQueues + kMaxPortMulticastQueues;
static constexpr uint32_t kMaxCpuQueues = 8;
static constexpr sai_object_id_t kEcmpHashId = 1234;
static constexpr sai_object_id_t kLagHashId = 1234;
static constexpr uint32_t kDefaultAclEntryMinimumPriority = 0;
static constexpr uint32_t kDefaultAclEntryMaximumPriority =
    std::numeric_limits<uint32_t>::max();

static constexpr uint32_t kDefaultFdbDstUserMetaDataRangeMin = 0;
static constexpr uint32_t kDefaultFdbDstUserMetaDataRangeMax =
    std::numeric_limits<uint32_t>::max();
static constexpr uint32_t kDefaultRouteDstUserMetaDataRangeMin = 0;
static constexpr uint32_t kDefaultRouteDstUserMetaDataRangeMax =
    std::numeric_limits<uint32_t>::max();
static constexpr uint32_t kDefaultNeighborDstUserMetaDataRangeMin = 0;
static constexpr uint32_t kDefaultNeighborDstUserMetaDataRangeMax =
    std::numeric_limits<uint32_t>::max();

} // namespace

sai_status_t facebook::fboss::FakeSwitch::setLed(const sai_attribute_t* attr) {
  switch (attr->id) {
    case SAI_SWITCH_ATTR_EXT_FAKE_LED: {
      if (attr->value.u32list.count != 4) {
        return SAI_STATUS_INVALID_PARAMETER;
      }
      auto id = attr->value.u32list.list[0];
      auto ledState = ledState_.find(id);
      if (ledState == ledState_.end()) {
        return SAI_STATUS_INVALID_ATTR_VALUE_0;
      }
      auto pgm = attr->value.u32list.list[1];
      if (pgm) {
        auto offset = attr->value.u32list.list[2];
        if (offset > 255) {
          return SAI_STATUS_INVALID_ATTR_VALUE_0 + 2;
        }
        ledState->second.program[offset] =
            static_cast<sai_uint8_t>(attr->value.u32list.list[3]);
      } else {
        auto index = attr->value.u32list.list[2];
        ledState->second.data[index] = attr->value.u32list.list[3];
      }
    } break;
    case SAI_SWITCH_ATTR_EXT_FAKE_LED_RESET: {
      if (attr->value.u32list.count != 2) {
        return SAI_STATUS_INVALID_PARAMETER;
      }
      auto id = attr->value.u32list.list[0];
      auto ledState = ledState_.find(id);
      if (ledState == ledState_.end()) {
        return SAI_STATUS_INVALID_ATTR_VALUE_0;
      }
      ledState->second.reset = bool(attr->value.u32list.list[1]);
    } break;
    default:
      return SAI_STATUS_UNKNOWN_ATTRIBUTE_0;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t facebook::fboss::FakeSwitch::getLed(sai_attribute_t* attr) const {
  switch (attr->id) {
    case SAI_SWITCH_ATTR_EXT_FAKE_LED: {
      if (attr->value.u32list.count != 4) {
        attr->value.u32list.count = 4;
        return SAI_STATUS_BUFFER_OVERFLOW;
      }
      auto id = attr->value.u32list.list[0];
      auto ledState = ledState_.find(id);
      if (ledState == ledState_.end()) {
        return SAI_STATUS_INVALID_ATTR_VALUE_0;
      }
      auto pgm = attr->value.u32list.list[1];
      if (pgm) {
        auto offset = attr->value.u32list.list[2];
        if (offset > 255) {
          return SAI_STATUS_INVALID_ATTR_VALUE_0;
        }
        attr->value.u32list.list[3] = ledState->second.program[offset];
      } else {
        auto index = attr->value.u32list.list[2];
        auto dataIter = ledState->second.data.find(index);
        attr->value.u32list.list[3] =
            (dataIter != ledState->second.data.end()) ? dataIter->second : 0;
      }
    } break;
    case SAI_SWITCH_ATTR_EXT_FAKE_LED_RESET: {
      if (attr->value.u32list.count != 2) {
        attr->value.u32list.count = 2;
        return SAI_STATUS_BUFFER_OVERFLOW;
      }
      auto id = attr->value.u32list.list[0];
      auto ledState = ledState_.find(id);
      if (ledState == ledState_.end()) {
        return SAI_STATUS_INVALID_ATTR_VALUE_0;
      }
      attr->value.u32list.list[1] = ((ledState->second.reset) ? 1 : 0);
    } break;
    default:
      return SAI_STATUS_UNKNOWN_ATTRIBUTE_0;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_switch_attribute_fn(
    sai_object_id_t switch_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& sw = fs->switchManager.get(switch_id);
  sai_status_t res;
  if (!attr) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  res = SAI_STATUS_SUCCESS;
  switch (attr->id) {
    case SAI_SWITCH_ATTR_SRC_MAC_ADDRESS:
      sw.setSrcMac(attr->value.mac);
      break;
    case SAI_SWITCH_ATTR_INIT_SWITCH:
      sw.setInitStatus(attr->value.booldata);
      break;
    case SAI_SWITCH_ATTR_SWITCH_HARDWARE_INFO:
      sw.setHwInfo(std::vector<int8_t>(
          attr->value.s8list.list,
          attr->value.s8list.list + attr->value.s8list.count));
      break;
    case SAI_SWITCH_ATTR_SWITCH_SHELL_ENABLE:
      sw.setShellStatus(attr->value.booldata);
      break;
    case SAI_SWITCH_ATTR_ECMP_HASH_IPV4:
      sw.setEcmpHashV4Id(attr->value.oid);
      break;
    case SAI_SWITCH_ATTR_ECMP_HASH_IPV6:
      sw.setEcmpHashV6Id(attr->value.oid);
      break;
    case SAI_SWITCH_ATTR_ECMP_DEFAULT_HASH_SEED:
      sw.setEcmpSeed(attr->value.u32);
      break;
    case SAI_SWITCH_ATTR_LAG_DEFAULT_HASH_SEED:
      sw.setLagSeed(attr->value.u32);
      break;
    case SAI_SWITCH_ATTR_ECMP_DEFAULT_HASH_ALGORITHM:
      sw.setEcmpAlgorithm(attr->value.s32);
      break;
    case SAI_SWITCH_ATTR_LAG_DEFAULT_HASH_ALGORITHM:
      sw.setLagAlgorithm(attr->value.s32);
      break;
    case SAI_SWITCH_ATTR_QOS_DSCP_TO_TC_MAP:
      sw.setDscpToTc(attr->value.oid);
      break;
    case SAI_SWITCH_ATTR_QOS_TC_TO_QUEUE_MAP:
      sw.setTcToQueue(attr->value.oid);
      break;
    case SAI_SWITCH_ATTR_RESTART_WARM:
      sw.setRestartWarm(attr->value.booldata);
      break;
    case SAI_SWITCH_ATTR_FDB_AGING_TIME:
      sw.setMacAgingTime(attr->value.u32);
      break;
    case SAI_SWITCH_ATTR_ECN_ECT_THRESHOLD_ENABLE:
      sw.setEcnEctThresholdEnable(attr->value.booldata);
      break;
    case SAI_SWITCH_ATTR_INGRESS_ACL:
      sw.setIngressAcl(attr->value.oid);
      break;
    case SAI_SWITCH_ATTR_EXT_FAKE_LED:
    case SAI_SWITCH_ATTR_EXT_FAKE_LED_RESET:
      return sw.setLed(attr);
    default:
      res = SAI_STATUS_INVALID_PARAMETER;
      break;
  }
  return res;
}

sai_status_t create_switch_fn(
    sai_object_id_t* switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  *switch_id = fs->switchManager.create();
  for (int i = 0; i < attr_count; ++i) {
    set_switch_attribute_fn(*switch_id, &attr_list[i]);
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_switch_fn(sai_object_id_t switch_id) {
  auto fs = FakeSai::getInstance();
  fs->switchManager.remove(switch_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_switch_attribute_fn(
    sai_object_id_t switch_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& sw = fs->switchManager.get(switch_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_SWITCH_ATTR_DEFAULT_1Q_BRIDGE_ID:
        attr[i].value.oid = kDefault1QBridgeId;
        break;
      case SAI_SWITCH_ATTR_DEFAULT_VIRTUAL_ROUTER_ID:
        attr[i].value.oid = kDefaultVirtualRouterId;
        break;
      case SAI_SWITCH_ATTR_DEFAULT_VLAN_ID:
        attr[i].value.oid = kDefaultVlanId;
        break;
      case SAI_SWITCH_ATTR_CPU_PORT:
        attr[i].value.oid = fs->getCpuPort();
        break;
      case SAI_SWITCH_ATTR_PORT_NUMBER:
        attr[i].value.u32 = fs->portManager.map().size();
        break;
      case SAI_SWITCH_ATTR_PORT_LIST: {
        if (fs->portManager.map().size() > attr[i].value.objlist.count) {
          attr[i].value.objlist.count = fs->portManager.map().size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        attr[i].value.objlist.count = fs->portManager.map().size();
        int j = 0;
        for (const auto& p : fs->portManager.map()) {
          attr[i].value.objlist.list[j++] = p.first;
        }
      } break;
      case SAI_SWITCH_ATTR_SRC_MAC_ADDRESS:
        std::copy_n(sw.srcMac().bytes(), 6, std::begin(attr[i].value.mac));
        break;
      case SAI_SWITCH_ATTR_INIT_SWITCH:
        attr[i].value.booldata = sw.isInitialized();
        break;
      case SAI_SWITCH_ATTR_SWITCH_HARDWARE_INFO:
        attr[i].value.s8list.count = sw.hwInfo().size();
        attr[i].value.s8list.list = sw.hwInfoData();
        break;
      case SAI_SWITCH_ATTR_SWITCH_SHELL_ENABLE:
        attr[i].value.booldata = sw.isShellEnabled();
        break;
      case SAI_SWITCH_ATTR_NUMBER_OF_UNICAST_QUEUES:
        attr[i].value.u32 = kMaxPortUnicastQueues;
        break;
      case SAI_SWITCH_ATTR_NUMBER_OF_MULTICAST_QUEUES:
        attr[i].value.u32 = kMaxPortMulticastQueues;
        break;
      case SAI_SWITCH_ATTR_NUMBER_OF_QUEUES:
        attr[i].value.u32 = kMaxPortQueues;
        break;
      case SAI_SWITCH_ATTR_NUMBER_OF_CPU_QUEUES:
        attr[i].value.u32 = kMaxCpuQueues;
        break;
      case SAI_SWITCH_ATTR_ECMP_HASH:
        attr[i].value.oid = kEcmpHashId;
        break;
      case SAI_SWITCH_ATTR_LAG_HASH:
        attr[i].value.oid = kLagHashId;
        break;
      case SAI_SWITCH_ATTR_ECMP_HASH_IPV4:
        attr[i].value.oid = sw.ecmpHashV4Id();
        break;
      case SAI_SWITCH_ATTR_ECMP_HASH_IPV6:
        attr[i].value.oid = sw.ecmpHashV6Id();
        break;
      case SAI_SWITCH_ATTR_ECMP_DEFAULT_HASH_SEED:
        attr[i].value.u32 = sw.ecmpSeed();
        break;
      case SAI_SWITCH_ATTR_LAG_DEFAULT_HASH_SEED:
        attr[i].value.u32 = sw.lagSeed();
        break;
      case SAI_SWITCH_ATTR_ECMP_DEFAULT_HASH_ALGORITHM:
        attr[i].value.s32 = sw.ecmpAlgorithm();
        break;
      case SAI_SWITCH_ATTR_LAG_DEFAULT_HASH_ALGORITHM:
        attr[i].value.s32 = sw.lagAlgorithm();
        break;
      case SAI_SWITCH_ATTR_RESTART_WARM:
        attr[i].value.booldata = sw.restartWarm();
        break;
      case SAI_SWITCH_ATTR_QOS_DSCP_TO_TC_MAP:
        attr[i].value.oid = sw.dscpToTc();
        break;
      case SAI_SWITCH_ATTR_QOS_TC_TO_QUEUE_MAP:
        attr[i].value.oid = sw.tcToQueue();
        break;
      case SAI_SWITCH_ATTR_ACL_ENTRY_MINIMUM_PRIORITY:
        attr[i].value.oid = kDefaultAclEntryMinimumPriority;
        break;
      case SAI_SWITCH_ATTR_ACL_ENTRY_MAXIMUM_PRIORITY:
        attr[i].value.oid = kDefaultAclEntryMaximumPriority;
        break;
      case SAI_SWITCH_ATTR_FDB_AGING_TIME:
        attr[i].value.u32 = sw.getMacAgingTime();
        break;
      case SAI_SWITCH_ATTR_FDB_DST_USER_META_DATA_RANGE:
        attr[i].value.u32range.min = kDefaultFdbDstUserMetaDataRangeMin;
        attr[i].value.u32range.max = kDefaultFdbDstUserMetaDataRangeMax;
        break;
      case SAI_SWITCH_ATTR_ROUTE_DST_USER_META_DATA_RANGE:
        attr[i].value.u32range.min = kDefaultRouteDstUserMetaDataRangeMin;
        attr[i].value.u32range.max = kDefaultRouteDstUserMetaDataRangeMax;
        break;
      case SAI_SWITCH_ATTR_NEIGHBOR_DST_USER_META_DATA_RANGE:
        attr[i].value.u32range.min = kDefaultNeighborDstUserMetaDataRangeMin;
        attr[i].value.u32range.max = kDefaultNeighborDstUserMetaDataRangeMax;
        break;
      case SAI_SWITCH_ATTR_ECN_ECT_THRESHOLD_ENABLE:
        attr[i].value.booldata = sw.getEcnEctThresholdEnable();
        break;
      case SAI_SWITCH_ATTR_EXT_FAKE_LED:
      case SAI_SWITCH_ATTR_EXT_FAKE_LED_RESET:
        return sw.getLed(attr);
      case SAI_SWITCH_ATTR_AVAILABLE_IPV4_ROUTE_ENTRY:
      case SAI_SWITCH_ATTR_AVAILABLE_IPV6_ROUTE_ENTRY:
      case SAI_SWITCH_ATTR_AVAILABLE_IPV4_NEXTHOP_ENTRY:
      case SAI_SWITCH_ATTR_AVAILABLE_IPV6_NEXTHOP_ENTRY:
      case SAI_SWITCH_ATTR_AVAILABLE_NEXT_HOP_GROUP_ENTRY:
      case SAI_SWITCH_ATTR_AVAILABLE_NEXT_HOP_GROUP_MEMBER_ENTRY:
      case SAI_SWITCH_ATTR_AVAILABLE_IPV4_NEIGHBOR_ENTRY:
      case SAI_SWITCH_ATTR_AVAILABLE_IPV6_NEIGHBOR_ENTRY:
        // Why not
        attr[i].value.u32 = 1'000'000;
        break;
      case SAI_SWITCH_ATTR_INGRESS_ACL:
        attr[i].value.oid = sw.getIngressAcl();
        break;

      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

static sai_switch_api_t _switch_api;

void populate_switch_api(sai_switch_api_t** switch_api) {
  _switch_api.create_switch = &create_switch_fn;
  _switch_api.remove_switch = &remove_switch_fn;
  _switch_api.set_switch_attribute = &set_switch_attribute_fn;
  _switch_api.get_switch_attribute = &get_switch_attribute_fn;
  *switch_api = &_switch_api;
}

} // namespace facebook::fboss
