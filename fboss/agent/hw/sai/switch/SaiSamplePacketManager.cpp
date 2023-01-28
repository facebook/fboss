/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiSamplePacketManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

#include "fboss/agent/platforms/sai/SaiPlatform.h"

namespace {
sai_samplepacket_type_t getSamplePacketType(
    facebook::fboss::cfg::SampleDestination sampleDestination) {
  switch (sampleDestination) {
    case facebook::fboss::cfg::SampleDestination::CPU:
      return SAI_SAMPLEPACKET_TYPE_SLOW_PATH;
    case facebook::fboss::cfg::SampleDestination::MIRROR:
      return SAI_SAMPLEPACKET_TYPE_MIRROR_SESSION;
    default:
      throw facebook::fboss::FbossError("sample destination is invalid");
  }
}
} // namespace
namespace facebook::fboss {

std::shared_ptr<SaiSamplePacket>
SaiSamplePacketManager::getOrCreateSamplePacket(
    uint64_t sampleRate,
    cfg::SampleDestination sampleDestination) {
  auto type = getSamplePacketType(sampleDestination);
  SaiSamplePacketTraits::AdapterHostKey k{
      sampleRate, type, SAI_SAMPLEPACKET_MODE_EXCLUSIVE};
  SaiSamplePacketTraits::CreateAttributes attributes = k;
  auto& store = saiStore_->get<SaiSamplePacketTraits>();
  return store.setObject(k, attributes);
}

SaiSamplePacketManager::SaiSamplePacketManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

} // namespace facebook::fboss
