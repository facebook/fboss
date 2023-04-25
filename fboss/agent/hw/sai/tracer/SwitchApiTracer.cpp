/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/SwitchApiTracer.h"
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _SwitchMap {
  SAI_ATTR_MAP(Switch, InitSwitch), SAI_ATTR_MAP(Switch, HwInfo),
      SAI_ATTR_MAP(Switch, SrcMac), SAI_ATTR_MAP(Switch, SwitchShellEnable),
      SAI_ATTR_MAP(Switch, EcmpHashV4), SAI_ATTR_MAP(Switch, EcmpHashV6),
      SAI_ATTR_MAP(Switch, LagHashV4), SAI_ATTR_MAP(Switch, LagHashV6),
      SAI_ATTR_MAP(Switch, EcmpDefaultHashSeed),
      SAI_ATTR_MAP(Switch, LagDefaultHashSeed),
      SAI_ATTR_MAP(Switch, EcmpDefaultHashAlgorithm),
      SAI_ATTR_MAP(Switch, LagDefaultHashAlgorithm),
      SAI_ATTR_MAP(Switch, SwitchRestartWarm), SAI_ATTR_MAP(Switch, CpuPort),
      SAI_ATTR_MAP(Switch, DefaultVlanId), SAI_ATTR_MAP(Switch, PortNumber),
      SAI_ATTR_MAP(Switch, PortList), SAI_ATTR_MAP(Switch, Default1QBridgeId),
      SAI_ATTR_MAP(Switch, DefaultVirtualRouterId),
      SAI_ATTR_MAP(Switch, NumberOfMulticastQueues),
      SAI_ATTR_MAP(Switch, LagHash), SAI_ATTR_MAP(Switch, EcmpHash),
      SAI_ATTR_MAP(Switch, QosDscpToTcMap),
      SAI_ATTR_MAP(Switch, QosTcToQueueMap),
      SAI_ATTR_MAP(Switch, QosExpToTcMap), SAI_ATTR_MAP(Switch, QosTcToExpMap),
      SAI_ATTR_MAP(Switch, AclEntryMinimumPriority),
      SAI_ATTR_MAP(Switch, AclEntryMaximumPriority),
      SAI_ATTR_MAP(Switch, MacAgingTime),
      SAI_ATTR_MAP(Switch, FdbDstUserMetaDataRange),
      SAI_ATTR_MAP(Switch, RouteDstUserMetaDataRange),
      SAI_ATTR_MAP(Switch, NeighborDstUserMetaDataRange),
      SAI_ATTR_MAP(Switch, AvailableIpv4RouteEntry),
      SAI_ATTR_MAP(Switch, AvailableIpv6RouteEntry),
      SAI_ATTR_MAP(Switch, AvailableIpv4NextHopEntry),
      SAI_ATTR_MAP(Switch, AvailableIpv6NextHopEntry),
      SAI_ATTR_MAP(Switch, AvailableNextHopGroupEntry),
      SAI_ATTR_MAP(Switch, AvailableNextHopGroupMemberEntry),
      SAI_ATTR_MAP(Switch, AvailableIpv4NeighborEntry),
      SAI_ATTR_MAP(Switch, AvailableIpv6NeighborEntry),
      SAI_ATTR_MAP(Switch, IngressAcl), SAI_ATTR_MAP(Switch, TamObject),
      SAI_ATTR_MAP(Switch, NumberOfFabricPorts),
      SAI_ATTR_MAP(Switch, FabricPortList),
      SAI_ATTR_MAP(Switch, UseEcnThresholds),
      SAI_ATTR_MAP(Switch, CounterRefreshInterval),
      SAI_ATTR_MAP(Switch, FirmwarePathName),
      SAI_ATTR_MAP(Switch, FirmwareLoadMethod),
      SAI_ATTR_MAP(Switch, FirmwareLoadType),
      SAI_ATTR_MAP(Switch, HardwareAccessBus),
      SAI_ATTR_MAP(Switch, PlatformContext),
      SAI_ATTR_MAP(Switch, SwitchProfileId),
      SAI_ATTR_MAP(Switch, FirmwareStatus),
      SAI_ATTR_MAP(Switch, FirmwareMajorVersion),
      SAI_ATTR_MAP(Switch, FirmwareMinorVersion),
      SAI_ATTR_MAP(Switch, PortConnectorList), SAI_ATTR_MAP(Switch, SwitchId),
      SAI_ATTR_MAP(Switch, MaxSystemCores),
      SAI_ATTR_MAP(Switch, SysPortConfigList), SAI_ATTR_MAP(Switch, SwitchType),
      SAI_ATTR_MAP(Switch, RegisterReadFn),
      SAI_ATTR_MAP(Switch, RegisterWriteFn),
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
      SAI_ATTR_MAP(Switch, MaxEcmpMemberCount),
      SAI_ATTR_MAP(Switch, EcmpMemberCount),
#endif
};

void handleExtensionAttributes() {
  SAI_EXT_ATTR_MAP(Switch, Led)
  SAI_EXT_ATTR_MAP(Switch, LedReset)
  SAI_EXT_ATTR_MAP(Switch, AclFieldList)
  SAI_EXT_ATTR_MAP(Switch, EgressPoolAvaialableSize)
  SAI_EXT_ATTR_MAP(Switch, HwEccErrorInitiate)
  SAI_EXT_ATTR_MAP(Switch, WarmBootTargetVersion)
}

} // namespace

namespace facebook::fboss {

sai_status_t wrap_create_switch(
    sai_object_id_t* switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto begin = FLAGS_enable_elapsed_time_log
      ? std::chrono::system_clock::now()
      : std::chrono::system_clock::time_point::min();
  auto rv = SaiTracer::getInstance()->switchApi_->create_switch(
      switch_id, attr_count, attr_list);
  SaiTracer::getInstance()->logSwitchCreateFn(switch_id, attr_count, attr_list);
  SaiTracer::getInstance()->logPostInvocation(rv, *switch_id, begin);
  return rv;
}

sai_status_t wrap_remove_switch(sai_object_id_t switch_id) {
  SaiTracer::getInstance()->logRemoveFn(
      "remove_switch", switch_id, SAI_OBJECT_TYPE_SWITCH);
  auto begin = FLAGS_enable_elapsed_time_log
      ? std::chrono::system_clock::now()
      : std::chrono::system_clock::time_point::min();
  auto rv = SaiTracer::getInstance()->switchApi_->remove_switch(switch_id);

  SaiTracer::getInstance()->logPostInvocation(rv, switch_id, begin);
  return rv;
}

sai_status_t wrap_set_switch_attribute(
    sai_object_id_t switch_id,
    const sai_attribute_t* attr) {
  sai_status_t rv{0};
  if (attr->id == SAI_SWITCH_ATTR_SWITCH_SHELL_ENABLE) {
    // this blocks forever, can't hold singleton or tracer must be leaky
    // singleton
    auto* tracer = SaiTracer::getInstance().get();
    rv = tracer->switchApi_->set_switch_attribute(switch_id, attr);
  } else {
    SaiTracer::getInstance()->logSetAttrFn(
        "set_switch_attribute", switch_id, attr, SAI_OBJECT_TYPE_SWITCH);
    auto begin = FLAGS_enable_elapsed_time_log
        ? std::chrono::system_clock::now()
        : std::chrono::system_clock::time_point::min();
    rv = SaiTracer::getInstance()->switchApi_->set_switch_attribute(
        switch_id, attr);
    SaiTracer::getInstance()->logPostInvocation(rv, switch_id, begin);
  }
  return rv;
}

sai_status_t wrap_get_switch_attribute(
    sai_object_id_t switch_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  if (FLAGS_enable_get_attr_log) {
    auto begin = FLAGS_enable_elapsed_time_log
        ? std::chrono::system_clock::now()
        : std::chrono::system_clock::time_point::min();
    auto rv = SaiTracer::getInstance()->switchApi_->get_switch_attribute(
        switch_id, attr_count, attr_list);
    SaiTracer::getInstance()->logGetAttrFn(
        "get_switch_attribute",
        switch_id,
        attr_count,
        attr_list,
        SAI_OBJECT_TYPE_SWITCH);
    SaiTracer::getInstance()->logPostInvocation(rv, switch_id, begin);
    return rv;
  }
  return SaiTracer::getInstance()->switchApi_->get_switch_attribute(
      switch_id, attr_count, attr_list);
}

sai_status_t wrap_get_switch_stats(
    sai_object_id_t switch_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    uint64_t* counters) {
  return SaiTracer::getInstance()->switchApi_->get_switch_stats(
      switch_id, number_of_counters, counter_ids, counters);
}

sai_status_t wrap_get_switch_stats_ext(
    sai_object_id_t switch_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    sai_stats_mode_t mode,
    uint64_t* counters) {
  return SaiTracer::getInstance()->switchApi_->get_switch_stats_ext(
      switch_id, number_of_counters, counter_ids, mode, counters);
}

sai_status_t wrap_clear_switch_stats(
    sai_object_id_t switch_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids) {
  return SaiTracer::getInstance()->switchApi_->clear_switch_stats(
      switch_id, number_of_counters, counter_ids);
}

sai_switch_api_t* wrappedSwitchApi() {
  handleExtensionAttributes();
  static sai_switch_api_t switchWrappers;

  switchWrappers.create_switch = &wrap_create_switch;
  switchWrappers.remove_switch = &wrap_remove_switch;
  switchWrappers.set_switch_attribute = &wrap_set_switch_attribute;
  switchWrappers.get_switch_attribute = &wrap_get_switch_attribute;
  switchWrappers.get_switch_stats = &wrap_get_switch_stats;
  switchWrappers.get_switch_stats_ext = &wrap_get_switch_stats_ext;
  switchWrappers.clear_switch_stats = &wrap_clear_switch_stats;

  return &switchWrappers;
}

SET_SAI_ATTRIBUTES(Switch)

} // namespace facebook::fboss
