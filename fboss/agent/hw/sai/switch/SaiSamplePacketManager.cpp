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

#include "fboss/agent/platforms/sai/SaiPlatform.h"

namespace {
sai_samplepacket_type_t getSamplePacketType(
    facebook::fboss::cfg::SampleDestination sampleDestination) {
  switch (sampleDestination) {
    case facebook::fboss::cfg::SampleDestination::CPU:
      return SAI_SAMPLEPACKET_TYPE_SLOW_PATH;
    case facebook::fboss::cfg::SampleDestination::MIRROR:
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 0)
      return SAI_SAMPLEPACKET_TYPE_MIRROR_SESSION;
#endif
    default:
      throw facebook::fboss::FbossError("sample destination is invalid");
  }
}
} // namespace
namespace facebook::fboss {

std::shared_ptr<SaiSamplePacket> SaiSamplePacketManager::createSamplePacket(
    uint64_t sampleRate,
    cfg::SampleDestination sampleDestination) {
  auto type = getSamplePacketType(sampleDestination);
  SaiSamplePacketTraits::AdapterHostKey k{
      sampleRate,
      type,
      SAI_SAMPLEPACKET_MODE_SHARED // TODO: Decide by asic support
  };
  SaiSamplePacketTraits::CreateAttributes attributes = k;
  auto& store = SaiStore::getInstance()->get<SaiSamplePacketTraits>();
  return store.setObject(k, attributes);
}

SaiSamplePacketManager::SaiSamplePacketManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {}

} // namespace facebook::fboss
