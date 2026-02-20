/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiDebugCounterManager.h"

#include "fboss/agent/hw/sai/api/DebugCounterApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

void SaiDebugCounterManager::setupDebugCounters() {
  setupPortL3BlackHoleCounter();
  setupMPLSLookupFailedCounter();
  setupAclDropCounter();
  setupEgressForwardingDropCounter();
  setupTrapDropCounter();
  setupL2SwitchDropCounter();
  setupL3SwitchDropCounter();
  setupTunnelSwitchDropCounter();
}

void SaiDebugCounterManager::setupPortL3BlackHoleCounter() {
  if (!platform_->getAsic()->isSupported(
          HwAsic::Feature::BLACKHOLE_ROUTE_DROP_COUNTER)) {
    return;
  }
  SaiInPortDebugCounterTraits::CreateAttributes attrs{
      SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS,
      SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
      SaiInPortDebugCounterTraits::Attributes::DropReasons{
          {SAI_IN_DROP_REASON_FDB_AND_BLACKHOLE_DISCARDS}}};

  auto& debugCounterStore = saiStore_->get<SaiInPortDebugCounterTraits>();
  portL3BlackHoleCounter_ = debugCounterStore.setObject(attrs, attrs);
  portL3BlackHoleCounterStatId_ = SAI_PORT_STAT_IN_DROP_REASON_RANGE_BASE +
      SaiApiTable::getInstance()->debugCounterApi().getAttribute(
          portL3BlackHoleCounter_->adapterKey(),
          SaiInPortDebugCounterTraits::Attributes::Index{});
}

void SaiDebugCounterManager::setupMPLSLookupFailedCounter() {
  bool mplsLookupFailCounterSupport = platform_->getAsic()->isSupported(
      HwAsic::Feature::SAI_MPLS_LABEL_LOOKUP_FAIL_COUNTER);
#if defined(TAJO_SDK_VERSION_1_42_8)
  mplsLookupFailCounterSupport = false;
#endif
  if (!mplsLookupFailCounterSupport) {
    return;
  }

#if SAI_API_VERSION >= SAI_VERSION(1, 9, 0)
  SaiInPortDebugCounterTraits::CreateAttributes attrs{
      SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS,
      SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
      SaiInPortDebugCounterTraits::Attributes::DropReasons{
          {SAI_IN_DROP_REASON_MPLS_MISS}}};
  auto& debugCounterStore = saiStore_->get<SaiInPortDebugCounterTraits>();
  mplsLookupFailCounter_ = debugCounterStore.setObject(attrs, attrs);
  mplsLookupFailCounterStatId_ = SAI_SWITCH_STAT_IN_DROP_REASON_RANGE_BASE +
      SaiApiTable::getInstance()->debugCounterApi().getAttribute(
          mplsLookupFailCounter_->adapterKey(),
          SaiInPortDebugCounterTraits::Attributes::Index{});
#endif
}

void SaiDebugCounterManager::setupAclDropCounter() {
  if (!platform_->getAsic()->isSupported(
          HwAsic::Feature::ANY_ACL_DROP_COUNTER)) {
    return;
  }
  SaiInPortDebugCounterTraits::CreateAttributes attrs{
      SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS,
      SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
      SaiInPortDebugCounterTraits::Attributes::DropReasons{
          {SAI_IN_DROP_REASON_ACL_ANY}}};
  auto& debugCounterStore = saiStore_->get<SaiInPortDebugCounterTraits>();
  aclDropCounter_ = debugCounterStore.setObject(attrs, attrs);
  aclDropCounterStatId_ = SAI_SWITCH_STAT_IN_DROP_REASON_RANGE_BASE +
      SaiApiTable::getInstance()->debugCounterApi().getAttribute(
          aclDropCounter_->adapterKey(),
          SaiInPortDebugCounterTraits::Attributes::Index{});
}

void SaiDebugCounterManager::setupTrapDropCounter() {
  if (!platform_->getAsic()->isSupported(
          HwAsic::Feature::ANY_TRAP_DROP_COUNTER)) {
    return;
  }
  SaiInPortDebugCounterTraits::CreateAttributes attrs{
      SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS,
      SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
      SaiInPortDebugCounterTraits::Attributes::DropReasons{
          {*SaiInPortDebugCounterTraits::trapDrops()}}};
  auto& debugCounterStore = saiStore_->get<SaiInPortDebugCounterTraits>();
  trapDropCounter_ = debugCounterStore.setObject(attrs, attrs);
  trapDropCounterStatId_ = SAI_SWITCH_STAT_IN_DROP_REASON_RANGE_BASE +
      SaiApiTable::getInstance()->debugCounterApi().getAttribute(
          trapDropCounter_->adapterKey(),
          SaiInPortDebugCounterTraits::Attributes::Index{});
}

void SaiDebugCounterManager::setupEgressForwardingDropCounter() {
  if (!platform_->getAsic()->isSupported(
          HwAsic::Feature::EGRESS_FORWARDING_DROP_COUNTER)) {
    return;
  }

  SaiOutPortDebugCounterTraits::CreateAttributes attrs{
      SAI_DEBUG_COUNTER_TYPE_PORT_OUT_DROP_REASONS,
      SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
      SaiOutPortDebugCounterTraits::Attributes::DropReasons{
          {SAI_OUT_DROP_REASON_L3_ANY}}};
  auto& debugCounterStore = saiStore_->get<SaiOutPortDebugCounterTraits>();
  egressForwardingDropCounter_ = debugCounterStore.setObject(attrs, attrs);
  egressForwardingDropCounterStatId_ =
      SAI_SWITCH_STAT_OUT_DROP_REASON_RANGE_BASE +
      SaiApiTable::getInstance()->debugCounterApi().getAttribute(
          egressForwardingDropCounter_->adapterKey(),
          SaiOutPortDebugCounterTraits::Attributes::Index{});
}

void SaiDebugCounterManager::setupL2SwitchDropCounter() {
  if (!platform_->getAsic()->isSupported(
          HwAsic::Feature::SWITCH_DROP_DEBUG_COUNTER)) {
    return;
  }

  /*
   * Generic L2 drop reasons not counted per-port
   */
  std::vector<int32_t> l2DropReasons = {
      SAI_IN_DROP_REASON_L2_ANY,
      SAI_IN_DROP_REASON_SMAC_MULTICAST,
      SAI_IN_DROP_REASON_SMAC_EQUALS_DMAC,
      SAI_IN_DROP_REASON_DMAC_RESERVED,
      SAI_IN_DROP_REASON_VLAN_TAG_NOT_ALLOWED,
      SAI_IN_DROP_REASON_INGRESS_VLAN_FILTER,
      SAI_IN_DROP_REASON_INGRESS_STP_FILTER,
      SAI_IN_DROP_REASON_FDB_UC_DISCARD,
      SAI_IN_DROP_REASON_FDB_MC_DISCARD,
      SAI_IN_DROP_REASON_L2_LOOPBACK_FILTER
#if SAI_API_VERSION >= SAI_VERSION(1, 17, 0) && defined(CHENAB_SAI_SDK)
      ,
      SAI_IN_DROP_REASON_SMAC_ZERO
#endif
  };

  SaiInSwitchDebugCounterTraits::CreateAttributes attrs{
      SAI_DEBUG_COUNTER_TYPE_SWITCH_IN_DROP_REASONS,
      SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
      SaiInSwitchDebugCounterTraits::Attributes::DropReasons{l2DropReasons}};
  auto& debugCounterStore = saiStore_->get<SaiInSwitchDebugCounterTraits>();
  l2SwitchDropCounter_ = debugCounterStore.setObject(attrs, attrs);
  l2SwitchDropCounterStatId_ = SAI_SWITCH_STAT_IN_DROP_REASON_RANGE_BASE +
      SaiApiTable::getInstance()->debugCounterApi().getAttribute(
          l2SwitchDropCounter_->adapterKey(),
          SaiInSwitchDebugCounterTraits::Attributes::Index{});
}

void SaiDebugCounterManager::setupL3SwitchDropCounter() {
  if (!platform_->getAsic()->isSupported(
          HwAsic::Feature::SWITCH_DROP_DEBUG_COUNTER)) {
    return;
  }

  /*
   * Generic L3 drop reasons not counted per-port.
   */
  std::vector<int32_t> l3DropReasons = {
      SAI_IN_DROP_REASON_L3_ANY,
      SAI_IN_DROP_REASON_EXCEEDS_L3_MTU,
      SAI_IN_DROP_REASON_TTL,
      SAI_IN_DROP_REASON_L3_LOOPBACK_FILTER,
      SAI_IN_DROP_REASON_NON_ROUTABLE,
      SAI_IN_DROP_REASON_NO_L3_HEADER,
      SAI_IN_DROP_REASON_IP_HEADER_ERROR,
      SAI_IN_DROP_REASON_UC_DIP_MC_DMAC,
      SAI_IN_DROP_REASON_DIP_LOOPBACK,
      SAI_IN_DROP_REASON_SIP_LOOPBACK,
      SAI_IN_DROP_REASON_SIP_MC,
      SAI_IN_DROP_REASON_SIP_CLASS_E,
      SAI_IN_DROP_REASON_SIP_UNSPECIFIED,
      SAI_IN_DROP_REASON_MC_DMAC_MISMATCH,
      SAI_IN_DROP_REASON_SIP_EQUALS_DIP,
      SAI_IN_DROP_REASON_SIP_BC,
      SAI_IN_DROP_REASON_DIP_LOCAL,
      SAI_IN_DROP_REASON_DIP_LINK_LOCAL,
      SAI_IN_DROP_REASON_SIP_LINK_LOCAL,
      SAI_IN_DROP_REASON_IPV6_MC_SCOPE0,
      SAI_IN_DROP_REASON_IPV6_MC_SCOPE1,
      SAI_IN_DROP_REASON_IRIF_DISABLED,
      SAI_IN_DROP_REASON_ERIF_DISABLED,
      SAI_IN_DROP_REASON_LPM4_MISS,
      SAI_IN_DROP_REASON_LPM6_MISS,
      SAI_IN_DROP_REASON_BLACKHOLE_ARP,
      SAI_IN_DROP_REASON_UNRESOLVED_NEXT_HOP};

  SaiInSwitchDebugCounterTraits::CreateAttributes attrs{
      SAI_DEBUG_COUNTER_TYPE_SWITCH_IN_DROP_REASONS,
      SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
      SaiInSwitchDebugCounterTraits::Attributes::DropReasons{l3DropReasons}};
  auto& debugCounterStore = saiStore_->get<SaiInSwitchDebugCounterTraits>();
  l3SwitchDropCounter_ = debugCounterStore.setObject(attrs, attrs);
  l3SwitchDropCounterStatId_ = SAI_SWITCH_STAT_IN_DROP_REASON_RANGE_BASE +
      SaiApiTable::getInstance()->debugCounterApi().getAttribute(
          l3SwitchDropCounter_->adapterKey(),
          SaiInSwitchDebugCounterTraits::Attributes::Index{});
}

void SaiDebugCounterManager::setupTunnelSwitchDropCounter() {
  if (!platform_->getAsic()->isSupported(
          HwAsic::Feature::SWITCH_DROP_DEBUG_COUNTER)) {
    return;
  }

  /*
   * Generic tunnel drop reasons not counted per-port.
   */
  std::vector<int32_t> tunnelDropReasons = {SAI_IN_DROP_REASON_DECAP_ERROR};

  SaiInSwitchDebugCounterTraits::CreateAttributes attrs{
      SAI_DEBUG_COUNTER_TYPE_SWITCH_IN_DROP_REASONS,
      SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
      SaiInSwitchDebugCounterTraits::Attributes::DropReasons{
          tunnelDropReasons}};
  auto& debugCounterStore = saiStore_->get<SaiInSwitchDebugCounterTraits>();
  tunnelSwitchDropCounter_ = debugCounterStore.setObject(attrs, attrs);
  tunnelSwitchDropCounterStatId_ = SAI_SWITCH_STAT_IN_DROP_REASON_RANGE_BASE +
      SaiApiTable::getInstance()->debugCounterApi().getAttribute(
          tunnelSwitchDropCounter_->adapterKey(),
          SaiInSwitchDebugCounterTraits::Attributes::Index{});
}

std::set<sai_stat_id_t> SaiDebugCounterManager::getConfiguredDebugStatIds()
    const {
  std::set<sai_stat_id_t> stats;
  if (portL3BlackHoleCounter_) {
    stats.insert(portL3BlackHoleCounterStatId_);
  }
  if (mplsLookupFailCounter_) {
    stats.insert(mplsLookupFailCounterStatId_);
  }
  if (aclDropCounter_) {
    stats.insert(aclDropCounterStatId_);
  }
  if (trapDropCounter_) {
    stats.insert(trapDropCounterStatId_);
  }
  if (egressForwardingDropCounter_) {
    stats.insert(egressForwardingDropCounterStatId_);
  }
  return stats;
}

std::set<sai_stat_id_t>
SaiDebugCounterManager::getConfiguredSwitchDebugStatIds() const {
  std::set<sai_stat_id_t> stats;
  if (l2SwitchDropCounter_) {
    stats.insert(l2SwitchDropCounterStatId_);
  }
  if (l3SwitchDropCounter_) {
    stats.insert(l3SwitchDropCounterStatId_);
  }
  if (tunnelSwitchDropCounter_) {
    stats.insert(tunnelSwitchDropCounterStatId_);
  }
  return stats;
}
} // namespace facebook::fboss
