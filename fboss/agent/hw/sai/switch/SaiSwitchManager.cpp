/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/HwSwitchFb303Stats.h"
#include "fboss/agent/hw/sai/api/AdapterKeySerializers.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/api/Types.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiBufferManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiUdfManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/switch_asics/Jericho3Asic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/LoadBalancer.h"
#include "fboss/agent/state/QosPolicy.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/logging/xlog.h>

extern "C" {
#include <sai.h>
}

DEFINE_uint32(
    counter_refresh_interval,
    1,
    "Counter refresh interval in seconds. Set it to 0 to fetch stats from HW");

DEFINE_bool(
    skip_setting_src_mac,
    false,
    "Flag to indicate whether to skip setting source mac in Sai switch during wb");

namespace {
using namespace facebook::fboss;
sai_hash_algorithm_t toSaiHashAlgo(cfg::HashingAlgorithm algo) {
  switch (algo) {
    case cfg::HashingAlgorithm::CRC:
      return SAI_HASH_ALGORITHM_CRC;
    case cfg::HashingAlgorithm::CRC16_CCITT:
      return SAI_HASH_ALGORITHM_CRC_CCITT;
    case cfg::HashingAlgorithm::CRC32_LO:
      return SAI_HASH_ALGORITHM_CRC_32LO;
    case cfg::HashingAlgorithm::CRC32_HI:
      return SAI_HASH_ALGORITHM_CRC_32HI;
    case cfg::HashingAlgorithm::CRC32_ETHERNET_LO:
    case cfg::HashingAlgorithm::CRC32_ETHERNET_HI:
    case cfg::HashingAlgorithm::CRC32_KOOPMAN_LO:
    case cfg::HashingAlgorithm::CRC32_KOOPMAN_HI:
    default:
      throw FbossError("Unsupported hash algorithm :", algo);
  }
}

bool isJerichoAsic(cfg::AsicType asicType) {
  return asicType == cfg::AsicType::ASIC_TYPE_JERICHO2 ||
      asicType == cfg::AsicType::ASIC_TYPE_JERICHO3;
}

void fillHwSwitchDropStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    HwSwitchDropStats& hwSwitchDropStats,
    cfg::AsicType asicType) {
  auto fillAsicSpecificCounter = [](auto counterId,
                                    auto val,
                                    auto asicType,
                                    auto& dropStats) {
    if (!isJerichoAsic(asicType)) {
      throw FbossError("Configured drop reason stats only supported for J2/J3");
    }
    switch (counterId) {
      /*
       * SAI_SWITCH_STAT_OUT_CONFIGURED_DROP_REASONS_0_DROPPED_PKTS -
       * FDR cell drops
       * SAI_SWITCH_STAT_OUT_CONFIGURED_DROP_REASONS_1_DROPPED_PKTS -
       * Reassembly drops due to corrupted cells
       * SAI_SWITCH_STAT_OUT_CONFIGURED_DROP_REASONS_2_DROPPED_PKTS -
       * Reassembly drops due to missing cells
       */
      case SAI_SWITCH_STAT_OUT_CONFIGURED_DROP_REASONS_0_DROPPED_PKTS:
        dropStats.fdrCellDrops() = val;
        break;
      case SAI_SWITCH_STAT_OUT_CONFIGURED_DROP_REASONS_1_DROPPED_PKTS:
        dropStats.corruptedCellPacketIntegrityDrops() = val;
        break;
      case SAI_SWITCH_STAT_OUT_CONFIGURED_DROP_REASONS_2_DROPPED_PKTS:
        dropStats.missingCellPacketIntegrityDrops() = val;
        break;
      /*
       * From CS00012306170
       * SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_0_DROPPED_PKTS - VOQ
       * resource exhaustion drops
       * SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_1_DROPPED_PKTS - Global
       * resource exhaustion drops
       * SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_2_DROPPED_PKTS - SRAM
       * resource exhaustion drops
       * SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_3_DROPPED_PKTS - VSQ
       * resource exhaustion drops
       * SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_4_DROPPED_PKTS - Drop
       * precedence drops
       * SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_5_DROPPED_PKTS - Queue
       * resolution drops
       * SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_6_DROPPED_PKTS - Ingress PP
       * VOQ drops due to PP reject bit
       */
      case SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_0_DROPPED_PKTS:
        dropStats.voqResourceExhaustionDrops() = val;
        break;
      case SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_1_DROPPED_PKTS:
        dropStats.globalResourceExhaustionDrops() = val;
        break;
      case SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_2_DROPPED_PKTS:
        dropStats.sramResourceExhaustionDrops() = val;
        break;
      case SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_3_DROPPED_PKTS:
        dropStats.vsqResourceExhaustionDrops() = val;
        break;
      case SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_4_DROPPED_PKTS:
        dropStats.dropPrecedenceDrops() = val;
        break;
      case SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_5_DROPPED_PKTS:
        dropStats.queueResolutionDrops() = val;
        break;
      case SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_6_DROPPED_PKTS:
        dropStats.ingressPacketPipelineRejectDrops() = val;
        break;
      default:
        throw FbossError("Unexpected configured counter id: ", counterId);
    }
  };
  for (auto counterIdAndValue : counterId2Value) {
    auto [counterId, value] = counterIdAndValue;
    switch (counterId) {
      case SAI_SWITCH_STAT_REACHABILITY_DROP:
        hwSwitchDropStats.globalReachabilityDrops() = value;
        break;
      case SAI_SWITCH_STAT_GLOBAL_DROP:
        hwSwitchDropStats.globalDrops() = value;
        break;
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
      case SAI_SWITCH_STAT_PACKET_INTEGRITY_DROP:
        hwSwitchDropStats.packetIntegrityDrops() = value;
        break;
#endif
      case SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_0_DROPPED_PKTS:
      case SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_1_DROPPED_PKTS:
      case SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_2_DROPPED_PKTS:
      case SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_3_DROPPED_PKTS:
      case SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_4_DROPPED_PKTS:
      case SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_5_DROPPED_PKTS:
      case SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_6_DROPPED_PKTS:
      case SAI_SWITCH_STAT_OUT_CONFIGURED_DROP_REASONS_0_DROPPED_PKTS:
      case SAI_SWITCH_STAT_OUT_CONFIGURED_DROP_REASONS_1_DROPPED_PKTS:
      case SAI_SWITCH_STAT_OUT_CONFIGURED_DROP_REASONS_2_DROPPED_PKTS:
        fillAsicSpecificCounter(counterId, value, asicType, hwSwitchDropStats);
        break;
      default:
        throw FbossError("Got unexpected switch counter id: ", counterId);
    }
  }
}
} // namespace

namespace facebook::fboss {

SaiSwitchManager::SaiSwitchManager(
    SaiManagerTable* managerTable,
    SaiPlatform* platform,
    BootType bootType,
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId)
    : managerTable_(managerTable), platform_(platform) {
  int64_t swId = switchId.value_or(0);
  switchPreInitSequence(platform->getAsic()->getAsicType());
  if (bootType == BootType::WARM_BOOT) {
    // Extract switch adapter key and create switch only with the mandatory
    // init attribute (warm boot path)
    auto& switchApi = SaiApiTable::getInstance()->switchApi();
    auto newSwitchId = switchApi.create<SaiSwitchTraits>(
        platform->getSwitchAttributes(true, switchType, switchId), swId);
    // Load all switch attributes
    switch_ = std::make_unique<SaiSwitchObj>(newSwitchId);
    if (switchType != cfg::SwitchType::FABRIC) {
      if (!FLAGS_skip_setting_src_mac) {
        switch_->setOptionalAttribute(
            SaiSwitchTraits::Attributes::SrcMac{platform->getLocalMac()});
      }
      switch_->setOptionalAttribute(SaiSwitchTraits::Attributes::MacAgingTime{
          platform->getDefaultMacAgingTime()});
    }
  } else {
    switch_ = std::make_unique<SaiSwitchObj>(
        std::monostate(),
        platform->getSwitchAttributes(false, switchType, switchId),
        swId);

    const auto& asic = platform_->getAsic();
    if (asic->isSupported(HwAsic::Feature::ECMP_HASH_V4)) {
      resetLoadBalancer<SaiSwitchTraits::Attributes::EcmpHashV4>();
    }
    if (asic->isSupported(HwAsic::Feature::ECMP_HASH_V6)) {
      resetLoadBalancer<SaiSwitchTraits::Attributes::EcmpHashV6>();
    }
    if (asic->isSupported(HwAsic::Feature::SAI_LAG_HASH)) {
      resetLoadBalancer<SaiSwitchTraits::Attributes::LagHashV4>();
      resetLoadBalancer<SaiSwitchTraits::Attributes::LagHashV6>();
    }
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
    if (asic->isSupported(HwAsic::Feature::ECMP_MEMBER_WIDTH_INTROSPECTION)) {
      auto maxEcmpCount = SaiApiTable::getInstance()->switchApi().getAttribute(
          switch_->adapterKey(),
          SaiSwitchTraits::Attributes::MaxEcmpMemberCount{});
      XLOG(DBG2) << "Got max ecmp member count " << maxEcmpCount;
      switch_->setOptionalAttribute(
          SaiSwitchTraits::Attributes::EcmpMemberCount{maxEcmpCount});
    }
#endif
  }
  if (platform_->getAsic()->isSupported(HwAsic::Feature::CPU_PORT)) {
    initCpuPort();
  }
  isMplsQosSupported_ = isMplsQoSMapSupported();
}

void SaiSwitchManager::initCpuPort() {
  cpuPort_ = SaiApiTable::getInstance()->switchApi().getAttribute(
      switch_->adapterKey(), SaiSwitchTraits::Attributes::CpuPort{});
}

PortSaiId SaiSwitchManager::getCpuPort() const {
  if (cpuPort_) {
    return *cpuPort_;
  }
  throw FbossError("getCpuPort not supported on Phy");
}

void SaiSwitchManager::setCpuRecyclePort(PortSaiId portSaiId) {
  cpuRecyclePort_ = portSaiId;
}

std::optional<PortSaiId> SaiSwitchManager::getCpuRecyclePort() const {
  return cpuRecyclePort_;
}

SwitchSaiId SaiSwitchManager::getSwitchSaiId() const {
  if (!switch_) {
    throw FbossError("failed to get switch id: switch not initialized");
  }
  return switch_->adapterKey();
}

void SaiSwitchManager::resetHashes() {
  if (ecmpV4Hash_) {
    switch_->setOptionalAttribute(
        SaiSwitchTraits::Attributes::EcmpHashV4{SAI_NULL_OBJECT_ID});
  }
  if (ecmpV6Hash_) {
    switch_->setOptionalAttribute(
        SaiSwitchTraits::Attributes::EcmpHashV6{SAI_NULL_OBJECT_ID});
  }
  if (lagV4Hash_) {
    switch_->setOptionalAttribute(
        SaiSwitchTraits::Attributes::LagHashV4{SAI_NULL_OBJECT_ID});
  }
  if (lagV6Hash_) {
    switch_->setOptionalAttribute(
        SaiSwitchTraits::Attributes::LagHashV6{SAI_NULL_OBJECT_ID});
  }
  ecmpV4Hash_.reset();
  ecmpV6Hash_.reset();
  lagV4Hash_.reset();
  lagV6Hash_.reset();
}

void SaiSwitchManager::resetQosMaps() {
  // Since Platform owns Asic, as well as SaiSwitch, which results
  // in blowing up asic before switch (due to destructor order details)
  // as a result, we can only rely on the validity of the global map pointer
  // to gate reset. This should only be true if resetting is supported and
  // would do something meaningful.
  if (globalDscpToTcQosMap_) {
    switch_->setOptionalAttribute(
        SaiSwitchTraits::Attributes::QosDscpToTcMap{SAI_NULL_OBJECT_ID});
    switch_->setOptionalAttribute(
        SaiSwitchTraits::Attributes::QosTcToQueueMap{SAI_NULL_OBJECT_ID});
    globalDscpToTcQosMap_.reset();
    globalTcToQueueQosMap_.reset();
  }
  if (isMplsQosSupported_) {
    switch_->setOptionalAttribute(
        SaiSwitchTraits::Attributes::QosExpToTcMap{SAI_NULL_OBJECT_ID});
    switch_->setOptionalAttribute(
        SaiSwitchTraits::Attributes::QosTcToExpMap{SAI_NULL_OBJECT_ID});
    if (globalExpToTcQosMap_) {
      globalExpToTcQosMap_.reset();
      globalTcToExpQosMap_.reset();
    }
  }
}

void SaiSwitchManager::programEcmpLoadBalancerParams(
    std::optional<sai_uint32_t> seed,
    std::optional<cfg::HashingAlgorithm> algo) {
  auto hashSeed = seed ? seed.value() : 0;
  auto hashAlgo = algo ? toSaiHashAlgo(algo.value()) : SAI_HASH_ALGORITHM_CRC;
  size_t kMaskLimit = sizeof(int) * kBitsPerByte;
  auto maxHashSeedLength = platform_->getAsic()->getMaxHashSeedLength();
  CHECK(maxHashSeedLength <= kMaskLimit);
  int mask = (maxHashSeedLength == kMaskLimit)
      ? -1
      : static_cast<int>(pow(2.0, maxHashSeedLength) - 1);
  hashSeed &= mask;
  switch_->setOptionalAttribute(
      SaiSwitchTraits::Attributes::EcmpDefaultHashSeed{hashSeed});
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::SAI_ECMP_HASH_ALGORITHM)) {
    switch_->setOptionalAttribute(
        SaiSwitchTraits::Attributes::EcmpDefaultHashAlgorithm{hashAlgo});
  }
}

template <typename HashAttrT>
SaiHashTraits::CreateAttributes SaiSwitchManager::getProgrammedHashAttr() {
  SaiHashTraits::CreateAttributes hashCreateAttrs{std::nullopt, std::nullopt};
  auto& [nativeHashFieldList, udfGroupList] = hashCreateAttrs;

  auto programmedHash =
      std::get<std::optional<HashAttrT>>(switch_->attributes());
  if (!programmedHash ||
      (programmedHash.value().value() == SAI_NULL_OBJECT_ID)) {
    return hashCreateAttrs;
  }

  auto programmedNativeHashFieldList =
      SaiApiTable::getInstance()->hashApi().getAttribute(
          HashSaiId{programmedHash.value().value()},
          SaiHashTraits::Attributes::NativeHashFieldList{});
  if (!programmedNativeHashFieldList.empty()) {
    nativeHashFieldList = programmedNativeHashFieldList;
  }

  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::UDF_HASH_FIELD_QUERY)) {
    auto programmedUDFGroupList =
        SaiApiTable::getInstance()->hashApi().getAttribute(
            HashSaiId{programmedHash.value().value()},
            SaiHashTraits::Attributes::UDFGroupList{});

    if (!programmedUDFGroupList.empty()) {
      udfGroupList = programmedUDFGroupList;
    }
  }

  return hashCreateAttrs;
}

template <typename HashAttrT>
void SaiSwitchManager::setLoadBalancer(
    const std::shared_ptr<SaiHash>& hash,
    SaiHashTraits::CreateAttributes& programmedHashCreateAttrs) {
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::SAI_HASH_FIELDS_CLEAR_BEFORE_SET)) {
    // This code patch is called during cold boot as well as warm boot but the
    // if check is true only in following cases:
    //     - during cold boot (very first time setting load balancer),
    //     - during warm boot (if config has load balancer changed post warm
    //     boot),
    //     - during reload config if load balancer config is updated.
    //
    // The check is false during warm boot if the config does not carry any
    // changes to the load balancer (common case).
    auto newHashCreateAttrs = hash->attributes();
    if (programmedHashCreateAttrs != newHashCreateAttrs) {
      resetLoadBalancer<HashAttrT>();
    }
  }

  switch_->setOptionalAttribute(HashAttrT{hash->adapterKey()});
}

template <typename HashAttrT>
void SaiSwitchManager::resetLoadBalancer() {
  // On some SAI implementations, setting new hash field has the effect of
  // setting hash fields to union of the current hash fields and the new
  // hash fields. This behavior is incorrect and will be fixed in the long
  // run (CS00012187635). In the meanwhile, clear and set desired fields as a
  // workaround.
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::SAI_HASH_FIELDS_CLEAR_BEFORE_SET)) {
    switch_->setOptionalAttribute(HashAttrT{SAI_NULL_OBJECT_ID});
  }
}

void SaiSwitchManager::addOrUpdateEcmpLoadBalancer(
    const std::shared_ptr<LoadBalancer>& newLb) {
  programEcmpLoadBalancerParams(newLb->getSeed(), newLb->getAlgorithm());

  // Get UdfGroup Ids if supported.
  auto udfGroupIds = getUdfGroupIds(newLb);

  if (newLb->getIPv4Fields().begin() != newLb->getIPv4Fields().end()) {
    // v4 ECMP
    auto programmedLoadBalancer =
        getProgrammedHashAttr<SaiSwitchTraits::Attributes::EcmpHashV4>();

    cfg::Fields v4EcmpHashFields;
    std::for_each(
        newLb->getIPv4Fields().begin(),
        newLb->getIPv4Fields().end(),
        [&v4EcmpHashFields](const auto& entry) {
          v4EcmpHashFields.ipv4Fields()->insert(entry->cref());
        });
    std::for_each(
        newLb->getTransportFields().begin(),
        newLb->getTransportFields().end(),
        [&v4EcmpHashFields](const auto& entry) {
          v4EcmpHashFields.transportFields()->insert(entry->cref());
        });

    ecmpV4Hash_ =
        managerTable_->hashManager().getOrCreate(v4EcmpHashFields, udfGroupIds);

    // Set the new ecmp v4 hash attribute on switch obj
    setLoadBalancer<SaiSwitchTraits::Attributes::EcmpHashV4>(
        ecmpV4Hash_, programmedLoadBalancer);
  }
  if (newLb->getIPv6Fields().begin() != newLb->getIPv6Fields().end()) {
    // v6 ECMP
    auto programmedLoadBalancer =
        getProgrammedHashAttr<SaiSwitchTraits::Attributes::EcmpHashV6>();

    cfg::Fields v6EcmpHashFields;
    std::for_each(
        newLb->getIPv6Fields().begin(),
        newLb->getIPv6Fields().end(),
        [&v6EcmpHashFields](const auto& entry) {
          v6EcmpHashFields.ipv6Fields()->insert(entry->cref());
        });

    std::for_each(
        newLb->getTransportFields().begin(),
        newLb->getTransportFields().end(),
        [&v6EcmpHashFields](const auto& entry) {
          v6EcmpHashFields.transportFields()->insert(entry->cref());
        });
    ecmpV6Hash_ =
        managerTable_->hashManager().getOrCreate(v6EcmpHashFields, udfGroupIds);

    // Set the new ecmp v6 hash attribute on switch obj
    setLoadBalancer<SaiSwitchTraits::Attributes::EcmpHashV6>(
        ecmpV6Hash_, programmedLoadBalancer);
  }
}

void SaiSwitchManager::programLagLoadBalancerParams(
    std::optional<sai_uint32_t> seed,
    std::optional<cfg::HashingAlgorithm> algo) {
  // TODO(skhare) setLoadBalancer is called only for ECMP today. Add similar
  // logic for LAG.
  auto hashSeed = seed ? seed.value() : 0;
  auto hashAlgo = algo ? toSaiHashAlgo(algo.value()) : SAI_HASH_ALGORITHM_CRC;
  switch_->setOptionalAttribute(
      SaiSwitchTraits::Attributes::LagDefaultHashSeed{hashSeed});
  switch_->setOptionalAttribute(
      SaiSwitchTraits::Attributes::LagDefaultHashAlgorithm{hashAlgo});
}

void SaiSwitchManager::addOrUpdateLagLoadBalancer(
    const std::shared_ptr<LoadBalancer>& newLb) {
  if (!platform_->getAsic()->isSupported(HwAsic::Feature::SAI_LAG_HASH)) {
    XLOG(WARN) << "Skip programming SAI_LAG_HASH, feature not supported ";
    return;
  }
  programLagLoadBalancerParams(newLb->getSeed(), newLb->getAlgorithm());

  if (newLb->getIPv4Fields().begin() != newLb->getIPv4Fields().end()) {
    // v4 LAG
    cfg::Fields v4LagHashFields;
    std::for_each(
        newLb->getIPv4Fields().begin(),
        newLb->getIPv4Fields().end(),
        [&v4LagHashFields](const auto& entry) {
          v4LagHashFields.ipv4Fields()->insert(entry->cref());
        });

    std::for_each(
        newLb->getTransportFields().begin(),
        newLb->getTransportFields().end(),
        [&v4LagHashFields](const auto& entry) {
          v4LagHashFields.transportFields()->insert(entry->cref());
        });
    lagV4Hash_ = managerTable_->hashManager().getOrCreate(v4LagHashFields);
    // Set the new lag v4 hash attribute on switch obj
    switch_->setOptionalAttribute(
        SaiSwitchTraits::Attributes::LagHashV4{lagV4Hash_->adapterKey()});
  }
  if (newLb->getIPv6Fields().begin() != newLb->getIPv6Fields().end()) {
    // v6 LAG
    cfg::Fields v6LagHashFields;
    std::for_each(
        newLb->getIPv6Fields().begin(),
        newLb->getIPv6Fields().end(),
        [&v6LagHashFields](const auto& entry) {
          v6LagHashFields.ipv6Fields()->insert(entry->cref());
        });

    std::for_each(
        newLb->getTransportFields().begin(),
        newLb->getTransportFields().end(),
        [&v6LagHashFields](const auto& entry) {
          v6LagHashFields.transportFields()->insert(entry->cref());
        });

    lagV6Hash_ = managerTable_->hashManager().getOrCreate(v6LagHashFields);
    // Set the new lag v6 hash attribute on switch obj
    switch_->setOptionalAttribute(
        SaiSwitchTraits::Attributes::LagHashV6{lagV6Hash_->adapterKey()});
  }
}

void SaiSwitchManager::addOrUpdateLoadBalancer(
    const std::shared_ptr<LoadBalancer>& newLb) {
  XLOG(DBG2) << "Add load balancer : " << newLb->getID();
  if (newLb->getID() == cfg::LoadBalancerID::AGGREGATE_PORT) {
    return addOrUpdateLagLoadBalancer(newLb);
  }
  addOrUpdateEcmpLoadBalancer(newLb);
}

void SaiSwitchManager::changeLoadBalancer(
    const std::shared_ptr<LoadBalancer>& /*oldLb*/,
    const std::shared_ptr<LoadBalancer>& newLb) {
  XLOG(DBG2) << "Change load balancer : " << newLb->getID();
  return addOrUpdateLoadBalancer(newLb);
}

void SaiSwitchManager::removeLoadBalancer(
    const std::shared_ptr<LoadBalancer>& oldLb) {
  XLOG(DBG2) << "Remove load balancer : " << oldLb->getID();
  if (oldLb->getID() == cfg::LoadBalancerID::AGGREGATE_PORT) {
    programLagLoadBalancerParams(std::nullopt, std::nullopt);
    resetLoadBalancer<SaiSwitchTraits::Attributes::LagHashV4>();
    resetLoadBalancer<SaiSwitchTraits::Attributes::LagHashV6>();
    lagV4Hash_.reset();
    lagV6Hash_.reset();
    return;
  }
  programEcmpLoadBalancerParams(std::nullopt, std::nullopt);
  resetLoadBalancer<SaiSwitchTraits::Attributes::EcmpHashV4>();
  resetLoadBalancer<SaiSwitchTraits::Attributes::EcmpHashV6>();
  ecmpV4Hash_.reset();
  ecmpV6Hash_.reset();
}

void SaiSwitchManager::setQosPolicy() {
  auto& qosMapManager = managerTable_->qosMapManager();
  XLOG(DBG2) << "Set default qos map";
  auto qosMapHandle = qosMapManager.getQosMap();
  // set switch attrs to oids
  if (isGlobalQoSMapSupported()) {
    globalDscpToTcQosMap_ = qosMapHandle->dscpToTcMap;
    switch_->setOptionalAttribute(SaiSwitchTraits::Attributes::QosDscpToTcMap{
        globalDscpToTcQosMap_->adapterKey()});
    globalTcToQueueQosMap_ = qosMapHandle->tcToQueueMap;
    switch_->setOptionalAttribute(SaiSwitchTraits::Attributes::QosTcToQueueMap{
        globalTcToQueueQosMap_->adapterKey()});
  }
  if (!isMplsQosSupported_) {
    return;
  }
  globalExpToTcQosMap_ = qosMapHandle->expToTcMap;
  if (globalExpToTcQosMap_) {
    switch_->setOptionalAttribute(SaiSwitchTraits::Attributes::QosExpToTcMap{
        globalExpToTcQosMap_->adapterKey()});
  }
  globalTcToExpQosMap_ = qosMapHandle->tcToExpMap;
  if (globalTcToExpQosMap_) {
    switch_->setOptionalAttribute(SaiSwitchTraits::Attributes::QosTcToExpMap{
        globalTcToExpQosMap_->adapterKey()});
  }
}

void SaiSwitchManager::clearQosPolicy() {
  XLOG(DBG2) << "Reset default qos map";
  resetQosMaps();
}

void SaiSwitchManager::setIngressAcl() {
  if (!platform_->getAsic()->isSupported(
          HwAsic::Feature::SWITCH_ATTR_INGRESS_ACL)) {
    return;
  }
  auto aclTableGroupHandle = managerTable_->aclTableGroupManager()
                                 .getAclTableGroupHandle(SAI_ACL_STAGE_INGRESS)
                                 ->aclTableGroup;
  setIngressAcl(aclTableGroupHandle->adapterKey());
}

void SaiSwitchManager::setIngressAcl(sai_object_id_t id) {
  if (!platform_->getAsic()->isSupported(
          HwAsic::Feature::SWITCH_ATTR_INGRESS_ACL)) {
    return;
  }
  XLOG(DBG2) << "Set ingress ACL; " << id;
  switch_->setOptionalAttribute(SaiSwitchTraits::Attributes::IngressAcl{id});
}

void SaiSwitchManager::resetIngressAcl() {
  // Since resetIngressAcl() will be called in SaiManagerTable destructor.,
  // we can't call pure virtual function HwAsic::isSupported() to check
  // whether an asic supports SAI_ACL_STAGE_INGRESS
  // Therefore, we will try to read from SaiObject to see whether there's a
  // ingress acl set. If there's a not null id set, we set it to null
  auto ingressAcl =
      std::get<std::optional<SaiSwitchTraits::Attributes::IngressAcl>>(
          switch_->attributes());
  if (ingressAcl && ingressAcl->value() != SAI_NULL_OBJECT_ID) {
    XLOG(DBG2) << "Reset current ingress acl:" << ingressAcl->value()
               << " back to null";
    switch_->setOptionalAttribute(
        SaiSwitchTraits::Attributes::IngressAcl{SAI_NULL_OBJECT_ID});
  }
}

void SaiSwitchManager::gracefulExit() {
  // On graceful exit we trigger the warm boot path on
  // ASIC by destroying the switch (and thus calling the
  // remove switch function
  // https://github.com/opencomputeproject/SAI/blob/master/inc/saiswitch.h#L2514
  // Other objects are left intact to preserve data plane
  // forwarding during warm boot
  switch_.reset();
}

void SaiSwitchManager::setMacAgingSeconds(sai_uint32_t agingSeconds) {
  switch_->setOptionalAttribute(
      SaiSwitchTraits::Attributes::MacAgingTime{agingSeconds});
}
sai_uint32_t SaiSwitchManager::getMacAgingSeconds() const {
  return GET_OPT_ATTR(Switch, MacAgingTime, switch_->attributes());
}

void SaiSwitchManager::setTamObject(std::vector<sai_object_id_t> tamObject) {
  switch_->setOptionalAttribute(
      SaiSwitchTraits::Attributes::TamObject{std::move(tamObject)});
}

void SaiSwitchManager::resetTamObject() {
  switch_->setOptionalAttribute(SaiSwitchTraits::Attributes::TamObject{
      std::vector<sai_object_id_t>{SAI_NULL_OBJECT_ID}});
}

void SaiSwitchManager::setArsProfile(
    [[maybe_unused]] ArsProfileSaiId arsProfileSaiId) {
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
  if (FLAGS_flowletSwitchingEnable &&
      platform_->getAsic()->isSupported(HwAsic::Feature::FLOWLET)) {
    switch_->setOptionalAttribute(
        SaiSwitchTraits::Attributes::ArsProfile{arsProfileSaiId});
  }
#endif
}

void SaiSwitchManager::resetArsProfile() {
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
  if (FLAGS_flowletSwitchingEnable &&
      platform_->getAsic()->isSupported(HwAsic::Feature::FLOWLET)) {
    switch_->setOptionalAttribute(
        SaiSwitchTraits::Attributes::ArsProfile{SAI_NULL_OBJECT_ID});
  }
#endif
}

void SaiSwitchManager::setupCounterRefreshInterval() {
  switch_->setOptionalAttribute(
      SaiSwitchTraits::Attributes::CounterRefreshInterval{
          FLAGS_counter_refresh_interval});
}

sai_object_id_t SaiSwitchManager::getDefaultVlanAdapterKey() const {
  return SaiApiTable::getInstance()->switchApi().getAttribute(
      switch_->adapterKey(), SaiSwitchTraits::Attributes::DefaultVlanId{});
}

void SaiSwitchManager::setPtpTcEnabled(bool ptpEnable) {
  isPtpTcEnabled_ = ptpEnable;
}

std::optional<bool> SaiSwitchManager::getPtpTcEnabled() {
  return isPtpTcEnabled_;
}

bool SaiSwitchManager::isGlobalQoSMapSupported() const {
#if defined(BRCM_SAI_SDK_XGS_AND_DNX)
  return false;
#endif
  return platform_->getAsic()->isSupported(HwAsic::Feature::QOS_MAP_GLOBAL);
}

bool SaiSwitchManager::isMplsQoSMapSupported() const {
#if defined(TAJO_SDK_VERSION_1_42_8)
  return false;
#endif
  return platform_->getAsic()->isSupported(HwAsic::Feature::SAI_MPLS_QOS);
}

const std::vector<sai_stat_id_t>& SaiSwitchManager::supportedDropStats() const {
  static std::vector<sai_stat_id_t> stats;
  if (stats.size()) {
    // initialized
    return stats;
  }
  if (platform_->getAsic()->isSupported(HwAsic::Feature::SWITCH_DROP_STATS)) {
    stats.insert(
        stats.end(),
        SaiSwitchTraits::CounterIdsToRead.begin(),
        SaiSwitchTraits::CounterIdsToRead.end());
    if (!platform_->getAsic()->isSupported(
            HwAsic::Feature::PACKET_INTEGRITY_DROP_STATS)) {
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
      stats.erase(std::find(
          stats.begin(), stats.end(), SAI_SWITCH_STAT_PACKET_INTEGRITY_DROP));
#endif
    }
    if (isJerichoAsic(platform_->getAsic()->getAsicType())) {
      static const std::vector<sai_stat_id_t> kJerichoConfigDropStats{
          SAI_SWITCH_STAT_OUT_CONFIGURED_DROP_REASONS_0_DROPPED_PKTS,
          SAI_SWITCH_STAT_OUT_CONFIGURED_DROP_REASONS_1_DROPPED_PKTS,
          SAI_SWITCH_STAT_OUT_CONFIGURED_DROP_REASONS_2_DROPPED_PKTS,
      };
      stats.insert(
          stats.end(),
          kJerichoConfigDropStats.begin(),
          kJerichoConfigDropStats.end());
    }
    if (platform_->getAsic()->getAsicType() ==
        cfg::AsicType::ASIC_TYPE_JERICHO3) {
      static const std::vector<sai_stat_id_t> kJericho3ConfigDropStats{
          // IN configured drop reasons
          SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_0_DROPPED_PKTS,
          SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_1_DROPPED_PKTS,
          SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_2_DROPPED_PKTS,
          SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_3_DROPPED_PKTS,
          SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_4_DROPPED_PKTS,
          SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_5_DROPPED_PKTS,
          SAI_SWITCH_STAT_IN_CONFIGURED_DROP_REASONS_6_DROPPED_PKTS,
      };
      stats.insert(
          stats.end(),
          kJericho3ConfigDropStats.begin(),
          kJericho3ConfigDropStats.end());
    }
  }
  return stats;
}

const std::vector<sai_stat_id_t>& SaiSwitchManager::supportedDramStats() const {
  static std::vector<sai_stat_id_t> stats;
  if (stats.size()) {
    // initialized
    return stats;
  }
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::DRAM_ENQUEUE_DEQUEUE_STATS)) {
    stats.insert(
        stats.end(),
        SaiSwitchTraits::dramStats().begin(),
        SaiSwitchTraits::dramStats().end());
  }
  if (platform_->getAsic()->isSupported(HwAsic::Feature::DRAM_BLOCK_TIME)) {
    stats.insert(
        stats.end(),
        SaiSwitchTraits::dramBlockTime().begin(),
        SaiSwitchTraits::dramBlockTime().end());
  }
  return stats;
}

const std::vector<sai_stat_id_t>& SaiSwitchManager::supportedWatermarkStats()
    const {
  static std::vector<sai_stat_id_t> stats;
  if (stats.size()) {
    // initialized
    return stats;
  }
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::RCI_WATERMARK_COUNTER)) {
    stats.insert(
        stats.end(),
        SaiSwitchTraits::rciWatermarkStats().begin(),
        SaiSwitchTraits::rciWatermarkStats().end());
  }
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::DTL_WATERMARK_COUNTER)) {
    stats.insert(
        stats.end(),
        SaiSwitchTraits::dtlWatermarkStats().begin(),
        SaiSwitchTraits::dtlWatermarkStats().end());
  }
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::EGRESS_CORE_BUFFER_WATERMARK)) {
    stats.insert(
        stats.end(),
        SaiSwitchTraits::egressCoreBufferWatermarkBytes().begin(),
        SaiSwitchTraits::egressCoreBufferWatermarkBytes().end());
  }
  return stats;
}

const std::vector<sai_stat_id_t>& SaiSwitchManager::supportedCreditStats()
    const {
  static std::vector<sai_stat_id_t> stats;
  if (stats.size()) {
    // initialized
    return stats;
  }
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::DELETED_CREDITS_STAT)) {
    stats.insert(
        stats.end(),
        SaiSwitchTraits::deletedCredits().begin(),
        SaiSwitchTraits::deletedCredits().end());
  }
  return stats;
}

const HwSwitchWatermarkStats SaiSwitchManager::getHwSwitchWatermarkStats()
    const {
  HwSwitchWatermarkStats switchWatermarkStats;
  // Get NPU specific watermark stats!
  auto supportedStats = supportedWatermarkStats();
  if (supportedStats.size()) {
    switch_->updateStats(supportedStats, SAI_STATS_MODE_READ_AND_CLEAR);
  }
  fillHwSwitchWatermarkStats(
      switch_->getStats(supportedStats), switchWatermarkStats);
  // SAI_SWITCH_STAT_DEVICE_WATERMARK_BYTES is always needed, however,
  // this stats as such is not supported as of now. Instead, the needed
  // watermarks at device level is fetched via the buffer pool watermark
  // SAI_BUFFER_POOL_STAT_WATERMARK_BYTES and available in SaiSwitch.
  switchWatermarkStats.deviceWatermarkBytes() =
      managerTable_->bufferManager().getDeviceWatermarkBytes();
  switchWatermarkStats.globalHeadroomWatermarkBytes()->insert(
      managerTable_->bufferManager().getGlobalHeadroomWatermarkBytes().begin(),
      managerTable_->bufferManager().getGlobalHeadroomWatermarkBytes().end());
  switchWatermarkStats.globalSharedWatermarkBytes()->insert(
      managerTable_->bufferManager().getGlobalSharedWatermarkBytes().begin(),
      managerTable_->bufferManager().getGlobalSharedWatermarkBytes().end());
  return switchWatermarkStats;
}

void SaiSwitchManager::updateStats(bool updateWatermarks) {
  auto switchDropStats = supportedDropStats();
  if (switchDropStats.size()) {
    switch_->updateStats(switchDropStats, SAI_STATS_MODE_READ);
    HwSwitchDropStats dropStats;
    fillHwSwitchDropStats(
        switch_->getStats(switchDropStats),
        dropStats,
        platform_->getAsic()->getAsicType());
    // Accumulate switch drop stats
    switchDropStats_.globalDrops() =
        switchDropStats_.globalDrops().value_or(0) +
        dropStats.globalDrops().value_or(0);
    switchDropStats_.globalReachabilityDrops() =
        switchDropStats_.globalReachabilityDrops().value_or(0) +
        dropStats.globalReachabilityDrops().value_or(0);
    switchDropStats_.packetIntegrityDrops() =
        switchDropStats_.packetIntegrityDrops().value_or(0) +
        dropStats.packetIntegrityDrops().value_or(0);
    switchDropStats_.fdrCellDrops() =
        switchDropStats_.fdrCellDrops().value_or(0) +
        dropStats.fdrCellDrops().value_or(0);
    switchDropStats_.voqResourceExhaustionDrops() =
        switchDropStats_.voqResourceExhaustionDrops().value_or(0) +
        dropStats.voqResourceExhaustionDrops().value_or(0);
    switchDropStats_.globalResourceExhaustionDrops() =
        switchDropStats_.globalResourceExhaustionDrops().value_or(0) +
        dropStats.globalResourceExhaustionDrops().value_or(0);
    switchDropStats_.sramResourceExhaustionDrops() =
        switchDropStats_.sramResourceExhaustionDrops().value_or(0) +
        dropStats.sramResourceExhaustionDrops().value_or(0);
    switchDropStats_.vsqResourceExhaustionDrops() =
        switchDropStats_.vsqResourceExhaustionDrops().value_or(0) +
        dropStats.vsqResourceExhaustionDrops().value_or(0);
    switchDropStats_.dropPrecedenceDrops() =
        switchDropStats_.dropPrecedenceDrops().value_or(0) +
        dropStats.dropPrecedenceDrops().value_or(0);
    switchDropStats_.queueResolutionDrops() =
        switchDropStats_.queueResolutionDrops().value_or(0) +
        dropStats.queueResolutionDrops().value_or(0);
    switchDropStats_.ingressPacketPipelineRejectDrops() =
        switchDropStats_.ingressPacketPipelineRejectDrops().value_or(0) +
        dropStats.ingressPacketPipelineRejectDrops().value_or(0);
    platform_->getHwSwitch()->getSwitchStats()->update(switchDropStats_);
  }
  auto switchDramStats = supportedDramStats();
  if (switchDramStats.size()) {
    switch_->updateStats(switchDramStats, SAI_STATS_MODE_READ_AND_CLEAR);
    HwSwitchDramStats dramStats;
    fillHwSwitchDramStats(switch_->getStats(switchDramStats), dramStats);
    platform_->getHwSwitch()->getSwitchStats()->update(dramStats);
  }
  auto switchCreditStats = supportedCreditStats();
  if (switchCreditStats.size()) {
    switch_->updateStats(switchCreditStats, SAI_STATS_MODE_READ_AND_CLEAR);
    HwSwitchCreditStats creditStats;
    fillHwSwitchCreditStats(switch_->getStats(switchCreditStats), creditStats);
    platform_->getHwSwitch()->getSwitchStats()->update(creditStats);
  }
  if (updateWatermarks) {
    switchWatermarkStats_ = getHwSwitchWatermarkStats();
    publishSwitchWatermarks(switchWatermarkStats_);
  }
}

void SaiSwitchManager::setSwitchIsolate(bool isolate) {
  // Supported only for FABRIC switches!
  // It is checked while applying thrift config
  CHECK(
      platform_->getAsic()->getSwitchType() == cfg::SwitchType::FABRIC ||
      platform_->getAsic()->getSwitchType() == cfg::SwitchType::VOQ);
  XLOG(DBG2) << " Setting switch state to : "
             << (isolate ? "DRAINED" : "UNDRAINED");
  switch_->setOptionalAttribute(
      SaiSwitchTraits::Attributes::SwitchIsolate{isolate});
}

std::vector<sai_object_id_t> SaiSwitchManager::getUdfGroupIds(
    const std::shared_ptr<LoadBalancer>& newLb) const {
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  if (platform_->getAsic()->isSupported(HwAsic::Feature::SAI_UDF_HASH)) {
    return managerTable_->udfManager().getUdfGroupIds(newLb->getUdfGroupIds());
  }
#endif
  return {};
}

void SaiSwitchManager::setForceTrafficOverFabric(bool forceTrafficOverFabric) {
  auto& switchApi = SaiApiTable::getInstance()->switchApi();
  auto oldSetForceTrafficOverFabric = switchApi.getAttribute(
      switch_->adapterKey(),
      SaiSwitchTraits::Attributes::ForceTrafficOverFabric{});
  if (oldSetForceTrafficOverFabric != forceTrafficOverFabric) {
    SaiApiTable::getInstance()->switchApi().setAttribute(
        switch_->adapterKey(),
        SaiSwitchTraits::Attributes::ForceTrafficOverFabric{
            forceTrafficOverFabric});
  }
}

void SaiSwitchManager::setCreditWatchdog(bool creditWatchdog) {
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  switch_->setOptionalAttribute(
      SaiSwitchTraits::Attributes::CreditWd{creditWatchdog});
#endif
}

void SaiSwitchManager::setLocalCapsuleSwitchIds(
    const std::map<SwitchID, int>& switchIdToNumCores) {
  std::vector<sai_uint32_t> values;
  for (const auto& it : switchIdToNumCores) {
    sai_uint32_t switchId = it.first;
    int numCores = it.second;
    for (auto idx = 0; idx < numCores; idx++) {
      values.push_back(switchId + idx);
    }
  }
  XLOG(DBG2) << "set local capsule switch ids "
             << std::string(values.begin(), values.end());
  if (values.empty()) {
#if defined BRCM_SAI_SDK_DNX_GTE_12_0
    switch_->setOptionalAttribute(
        SaiSwitchTraits::Attributes::MultiStageLocalSwitchIds{values});
#endif
    // Note: setting MultiStageLocalSwitchIds to empty, when its was not set
    // before should be a no-op, but there is a bug in pre 12.0 sai where this
    // is not correctly handled complaining invalid parameters
    return;
  }
  switch_->setOptionalAttribute(
      SaiSwitchTraits::Attributes::MultiStageLocalSwitchIds{values});
}

void SaiSwitchManager::setReachabilityGroupList(int reachabilityGroupListSize) {
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
  if (reachabilityGroupListSize > 0) {
    std::vector<uint32_t> list;
    for (int i = 0; i < reachabilityGroupListSize; i++) {
      list.push_back(i + 1);
    }
    switch_->setOptionalAttribute(
        SaiSwitchTraits::Attributes::ReachabilityGroupList{list});
  }
#endif
}
} // namespace facebook::fboss
