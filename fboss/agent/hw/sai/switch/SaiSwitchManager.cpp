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
#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/LoadBalancer.h"
#include "fboss/agent/state/StateDelta.h"

#include <folly/logging/xlog.h>

extern "C" {
#include <sai.h>
}
namespace {
using namespace facebook::fboss;

// (TODO: srikrishnagopu) Move this to SaiPlatform ?
SaiSwitchTraits::CreateAttributes getSwitchAttributes(SaiPlatform* platform) {
  std::vector<int8_t> connectionHandle;
  auto connStr = platform->getPlatformAttribute(
      cfg::PlatformAttributes::CONNECTION_HANDLE);
  if (connStr.has_value()) {
    std::copy(
        connStr->c_str(),
        connStr->c_str() + connStr->size() + 1,
        std::back_inserter(connectionHandle));
  }
  SaiSwitchTraits::Attributes::InitSwitch initSwitch(true);
  SaiSwitchTraits::Attributes::HwInfo hwInfo(connectionHandle);
  SaiSwitchTraits::Attributes::SrcMac srcMac(platform->getLocalMac());
  return {
      initSwitch,
      hwInfo,
      srcMac,
      std::nullopt, // shell
      std::nullopt, // ecmp hash v4
      std::nullopt, // ecmp hash v6
      std::nullopt, // ecmp hash seed
      std::nullopt, // lag hash seed
      std::nullopt, // ecmp hash algo
      std::nullopt, // lag hash algo
      std::nullopt, // restart warm
  };
}

sai_hash_algorithm_t toSaiHashAlgo(cfg::HashingAlgorithm algo) {
  switch (algo) {
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
      throw FbossError("Unsupported hash algorithm :", algo);
  }
}
} // namespace

namespace facebook::fboss {

SaiSwitchInstance::SaiSwitchInstance(
    const SaiSwitchTraits::CreateAttributes& attributes)
    : switch_(std::monostate(), attributes, 0 /* fake switch id; ignored */) {}

SaiSwitchManager::SaiSwitchManager(
    SaiManagerTable* managerTable,
    SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {
  switch_ = std::make_unique<SaiSwitchInstance>(getSwitchAttributes(platform));
}

SaiSwitchInstance* SaiSwitchManager::getSwitchImpl() const {
  if (!switch_) {
    XLOG(FATAL) << "invalid null switch";
  }
  return switch_.get();
}

const SaiSwitchInstance* SaiSwitchManager::getSwitch() const {
  return getSwitchImpl();
}

SaiSwitchInstance* SaiSwitchManager::getSwitch() {
  return getSwitchImpl();
}

SwitchSaiId SaiSwitchManager::getSwitchSaiId() const {
  auto switchInstance = getSwitch();
  if (!switchInstance) {
    throw FbossError("failed to get switch id: switch not initialized");
  }
  return switchInstance->id();
}

void SaiSwitchManager::resetHashes() {
  ecmpV4Hash_.reset();
  ecmpV6Hash_.reset();
}

void SaiSwitchManager::programLoadBalancerParams(
    cfg::LoadBalancerID /*id*/,
    std::optional<sai_uint32_t> seed,
    std::optional<cfg::HashingAlgorithm> algo) {
  auto hashSeed = seed ? seed.value() : 0;
  auto& switchApi = SaiApiTable::getInstance()->switchApi();
  switchApi.setAttribute(
      getSwitchSaiId(),
      SaiSwitchTraits::Attributes::EcmpDefaultHashSeed{hashSeed});
  auto hashAlgo = algo ? toSaiHashAlgo(algo.value()) : SAI_HASH_ALGORITHM_CRC;
  switchApi.setAttribute(
      getSwitchSaiId(),
      SaiSwitchTraits::Attributes::EcmpDefaultHashAlgorithm{hashAlgo});
}

void SaiSwitchManager::addOrUpdateLoadBalancer(
    const std::shared_ptr<LoadBalancer>& newLb) {
  if (newLb->getID() == cfg::LoadBalancerID::AGGREGATE_PORT) {
    throw FbossError("Hash configuration for aggregate ports is not supported");
  }
  programLoadBalancerParams(
      newLb->getID(), newLb->getSeed(), newLb->getAlgorithm());
  if (newLb->getIPv4Fields().size()) {
    // v4 ECMP
    cfg::Fields v4EcmpHashFields;
    v4EcmpHashFields.ipv4Fields.insert(
        newLb->getIPv4Fields().begin(), newLb->getIPv4Fields().end());
    v4EcmpHashFields.transportFields.insert(
        newLb->getTransportFields().begin(), newLb->getTransportFields().end());
    ecmpV4Hash_ = managerTable_->hashManager().getOrCreate(v4EcmpHashFields);
    auto& switchApi = SaiApiTable::getInstance()->switchApi();
    switchApi.setAttribute(
        getSwitchSaiId(),
        SaiSwitchTraits::Attributes::EcmpHashV4(ecmpV4Hash_->adapterKey()));
  }
  if (newLb->getIPv6Fields().size()) {
    // v6 ECMP
    cfg::Fields v6EcmpHashFields;
    v6EcmpHashFields.ipv6Fields.insert(
        newLb->getIPv6Fields().begin(), newLb->getIPv6Fields().end());
    v6EcmpHashFields.transportFields.insert(
        newLb->getTransportFields().begin(), newLb->getTransportFields().end());
    ecmpV6Hash_ = managerTable_->hashManager().getOrCreate(v6EcmpHashFields);
    auto& switchApi = SaiApiTable::getInstance()->switchApi();
    switchApi.setAttribute(
        getSwitchSaiId(),
        SaiSwitchTraits::Attributes::EcmpHashV6(ecmpV6Hash_->adapterKey()));
  }
}

void SaiSwitchManager::removeLoadBalancer(
    const std::shared_ptr<LoadBalancer>& oldLb) {
  if (oldLb->getID() == cfg::LoadBalancerID::AGGREGATE_PORT) {
    throw FbossError("Hash configuration for Agg ports is not supported");
  }
  programLoadBalancerParams(oldLb->getID(), std::nullopt, std::nullopt);
  ecmpV4Hash_.reset();
  ecmpV6Hash_.reset();
}

void SaiSwitchManager::processLoadBalancerDelta(const StateDelta& delta) {
  DeltaFunctions::forEachChanged(
      delta.getLoadBalancersDelta(),
      [this](
          const std::shared_ptr<LoadBalancer>& /*oldLb*/,
          const std::shared_ptr<LoadBalancer>& newLb) {
        addOrUpdateLoadBalancer(newLb);
      },
      [this](const std::shared_ptr<LoadBalancer>& add) {
        addOrUpdateLoadBalancer(add);
      },
      [this](const std::shared_ptr<LoadBalancer>& remove) {
        removeLoadBalancer(remove);
      });
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
} // namespace facebook::fboss
