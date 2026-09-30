/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiVirtualChannelManager.h"

#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/agent/state/Port.h"

namespace facebook::fboss {

SaiVirtualChannelManager::SaiVirtualChannelManager(
    SaiStore* saiStore,
    const SaiPlatform* platform)
    : saiStore_(saiStore), platform_(platform) {}

#if defined(SAI_CBFC_SUPPORTED)

std::shared_ptr<SaiCbfcCreditProfile>
SaiVirtualChannelManager::getOrCreateCreditProfile(int64_t reservedCreditSize) {
  // POOL_ID is MANDATORY_ON_CREATE and must be null: CBFC_CREDIT_POOL is
  // unsupported on 16.0_ea_odp, so no pool object can exist to reference.
  SaiCbfcCreditProfileTraits::CreateAttributes attributes{
      SAI_NULL_OBJECT_ID, static_cast<sai_uint64_t>(reservedCreditSize)};
  return saiStore_->get<SaiCbfcCreditProfileTraits>().setObject(
      attributes, attributes);
}

#endif

void SaiVirtualChannelManager::programVirtualChannels(
    [[maybe_unused]] const std::shared_ptr<Port>& swPort,
    [[maybe_unused]] PortSaiId portSaiId) {
#if defined(SAI_CBFC_SUPPORTED)
  if (!platform_->getAsic()->isSupported(HwAsic::Feature::CBFC)) {
    return;
  }

  // Reconciled unconditionally, and ahead of the virtualChannels guard below:
  // 0 is the SDK's "no port credit limit", so an unset config value has to be
  // written rather than skipped. Skipping it leaves the last value programmed
  // forever -- on a port that drops only its limit, and on a port that drops
  // CBFC entirely and returns early below -- which a later config re-enabling
  // CBFC without a limit would then silently inherit.
  //
  // Set through the port api rather than the port's CreateAttributes.
  // brcm-sai 16.0_ea_odp rejects a GET of this attribute, and SaiStore reads
  // back every attribute in that tuple for every port at init, so including it
  // there aborts the HW agent on boot even with no CBFC configured. That same
  // failed GET is why nothing can read this back to verify it.
  SaiApiTable::getInstance()->portApi().setAttribute(
      portSaiId,
      SaiPortTraits::Attributes::CbfcSenderCreditLimit{
          static_cast<sai_uint32_t>(
              swPort->getCbfcSenderCreditLimit().value_or(0))});

  using Attributes = SaiVirtualChannelTraits::Attributes;

  SaiVirtualChannelHandle handle;
  auto& store = saiStore_->get<SaiVirtualChannelTraits>();
  // virtualChannels is an optional thrift field, and ApplyThriftConfig leaves
  // it unset on a port that names no cbfcConfig. Erasing is what an empty list
  // would do anyway.
  const auto& virtualChannels = swPort->getVirtualChannels();
  if (!virtualChannels) {
    handles_.erase(swPort->getID());
    return;
  }

  for (const auto& virtualChannel : *virtualChannels) {
    // THRIFT_COPY
    auto vc = virtualChannel->toThrift();

    std::optional<Attributes::CbfcSenderCreditProfile> creditProfile;
    if (auto reservedCreditSize = vc.reservedCreditSize()) {
      auto profile = getOrCreateCreditProfile(*reservedCreditSize);
      creditProfile =
          Attributes::CbfcSenderCreditProfile{profile->adapterKey()};
      handle.creditProfiles.push_back(std::move(profile));
    }

    SaiVirtualChannelTraits::CreateAttributes attributes{
        portSaiId,
        static_cast<sai_uint8_t>(*vc.id()),
        creditProfile,
        *vc.receiverEnable(),
        *vc.senderEnable()};
    SaiVirtualChannelTraits::AdapterHostKey key = tupleProjection<
        SaiVirtualChannelTraits::CreateAttributes,
        SaiVirtualChannelTraits::AdapterHostKey>(attributes);
    // Unconditional, so that on warm boot this reclaims the object the store
    // loaded from hardware and it is not swept as unreferenced.
    handle.virtualChannels.push_back(store.setObject(key, attributes));
  }

  // Assigning last is what removes anything the port no longer wants: the
  // objects above are already created and bound, so the profile references
  // dropped here are only the stale ones.
  handles_[swPort->getID()] = std::move(handle);
#endif
}

#if defined(SAI_CBFC_SUPPORTED)
const SaiVirtualChannelHandle*
SaiVirtualChannelManager::getVirtualChannelHandle(PortID portId) const {
  auto itr = handles_.find(portId);
  return itr == handles_.end() ? nullptr : &itr->second;
}
#endif

void SaiVirtualChannelManager::removeVirtualChannels(
    [[maybe_unused]] PortID portId) {
#if defined(SAI_CBFC_SUPPORTED)
  handles_.erase(portId);
#endif
}

} // namespace facebook::fboss
