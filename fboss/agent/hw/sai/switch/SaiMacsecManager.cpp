/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiMacsecManager.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/CounterUtils.h"
#include "fboss/agent/hw/sai/api/SaiDefaultAttributeValues.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"

#include <folly/logging/xlog.h>
#include "fboss/agent/FbossError.h"

extern "C" {
#include <sai.h>
}

namespace {
using namespace facebook::fboss;
constexpr sai_uint8_t kSectagOffset = 12;
constexpr sai_int32_t kReplayProtectionWindow = 100;

static std::array<uint8_t, 12> kDefaultSaltValue{
    0x9d,
    0x00,
    0x29,
    0x02,
    0x48,
    0xde,
    0x86,
    0xa2,
    0x1c,
    0x66,
    0xfa,
    0x6d};
const MacsecShortSecureChannelId kDefaultSsciValue =
    MacsecShortSecureChannelId(0x01000000);

// ACL priority values for MacSec (lower number has higher priority)
constexpr int kMacsecDefaultAclPriority = 4;
constexpr int kMacsecAclPriority = 3;
constexpr int kMacsecLldpAclPriority = 2;
constexpr int kMacsecMkaAclPriority = 1;

const std::shared_ptr<AclEntry> createMacsecAclEntry(
    int priority,
    std::string entryName,
    MacsecFlowSaiId flowId,
    std::optional<folly::MacAddress> mac,
    std::optional<cfg::EtherType> etherType) {
  auto entry = std::make_shared<AclEntry>(priority, entryName);
  if (mac.has_value()) {
    entry->setDstMac(*mac);
  }
  if (etherType.has_value()) {
    entry->setEtherType(*etherType);
  }
  auto macsecAction = MatchAction();
  auto macsecFlowAction = cfg::MacsecFlowAction();
  macsecFlowAction.action_ref() = cfg::MacsecFlowPacketAction::MACSEC_FLOW;
  macsecFlowAction.flowId_ref() = flowId;
  macsecAction.setMacsecFlow(macsecFlowAction);
  entry->setActionType(cfg::AclActionType::PERMIT);
  entry->setAclAction(macsecAction);

  return entry;
}

const std::shared_ptr<AclEntry> createMacsecControlAclEntry(
    int priority,
    std::string entryName,
    std::optional<folly::MacAddress> mac,
    std::optional<cfg::EtherType> etherType) {
  auto entry = std::make_shared<AclEntry>(priority, entryName);
  if (mac.has_value()) {
    entry->setDstMac(*mac);
  }
  if (etherType.has_value()) {
    entry->setEtherType(*etherType);
  }

  auto macsecAction = MatchAction();
  auto macsecFlowAction = cfg::MacsecFlowAction();
  macsecFlowAction.action_ref() = cfg::MacsecFlowPacketAction::FORWARD;
  macsecFlowAction.flowId_ref() = 0;
  macsecAction.setMacsecFlow(macsecFlowAction);
  entry->setActionType(cfg::AclActionType::PERMIT);
  entry->setAclAction(macsecAction);

  return entry;
}

const std::shared_ptr<AclEntry> createMacsecRxDefaultAclEntry(
    int priority,
    std::string entryName,
    cfg::MacsecFlowPacketAction action) {
  auto entry = std::make_shared<AclEntry>(priority, entryName);

  auto macsecAction = MatchAction();
  auto macsecFlowAction = cfg::MacsecFlowAction();
  macsecFlowAction.action_ref() = action;
  macsecFlowAction.flowId_ref() = 0;
  macsecAction.setMacsecFlow(macsecFlowAction);
  if (action == cfg::MacsecFlowPacketAction::DROP) {
    entry->setActionType(cfg::AclActionType::DENY);
  } else {
    entry->setActionType(cfg::AclActionType::PERMIT);
  }

  // Add traffic counter for packet hitting default ACL
  auto counter = cfg::TrafficCounter();
  counter.name_ref() =
      folly::to<std::string>(entryName, ".", priority, ".defaultAcl.packets");
  counter.types_ref() = {cfg::CounterType::PACKETS};
  macsecAction.setTrafficCounter(counter);
  entry->setAclAction(macsecAction);

  return entry;
}

void fillHwPortStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    mka::MacsecPortStats& portStats) {
  for (auto counterIdAndValue : counterId2Value) {
    auto [counterId, value] = counterIdAndValue;
    switch (counterId) {
      case SAI_MACSEC_PORT_STAT_PRE_MACSEC_DROP_PKTS:
        portStats.preMacsecDropPkts_ref() = value;
        break;
      case SAI_MACSEC_PORT_STAT_CONTROL_PKTS:
        portStats.controlPkts_ref() = value;
        break;
      case SAI_MACSEC_PORT_STAT_DATA_PKTS:
        portStats.dataPkts_ref() = value;
        break;
    }
  }
}
mka::MacsecFlowStats fillFlowStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    sai_macsec_direction_t direction) {
  mka::MacsecFlowStats flowStats{};
  flowStats.directionIngress_ref() = direction == SAI_MACSEC_DIRECTION_INGRESS;
  for (auto counterIdAndValue : counterId2Value) {
    auto [counterId, value] = counterIdAndValue;
    switch (counterId) {
      case SAI_MACSEC_FLOW_STAT_UCAST_PKTS_UNCONTROLLED:
        flowStats.ucastUncontrolledPkts_ref() = value;
        break;
      case SAI_MACSEC_FLOW_STAT_UCAST_PKTS_CONTROLLED:
        flowStats.ucastControlledPkts_ref() = value;
        break;
      case SAI_MACSEC_FLOW_STAT_MULTICAST_PKTS_UNCONTROLLED:
        flowStats.mcastUncontrolledPkts_ref() = value;
        break;
      case SAI_MACSEC_FLOW_STAT_MULTICAST_PKTS_CONTROLLED:
        flowStats.mcastControlledPkts_ref() = value;
        break;
      case SAI_MACSEC_FLOW_STAT_BROADCAST_PKTS_UNCONTROLLED:
        flowStats.bcastUncontrolledPkts_ref() = value;
        break;
      case SAI_MACSEC_FLOW_STAT_BROADCAST_PKTS_CONTROLLED:
        flowStats.bcastControlledPkts_ref() = value;
        break;
      case SAI_MACSEC_FLOW_STAT_CONTROL_PKTS:
        flowStats.controlPkts_ref() = value;
        break;
      case SAI_MACSEC_FLOW_STAT_PKTS_UNTAGGED:
        flowStats.untaggedPkts_ref() = value;
        break;
      case SAI_MACSEC_FLOW_STAT_OTHER_ERR:
        flowStats.otherErrPkts_ref() = value;
        break;
      case SAI_MACSEC_FLOW_STAT_OCTETS_UNCONTROLLED:
        flowStats.octetsUncontrolled_ref() = value;
        break;
      case SAI_MACSEC_FLOW_STAT_OCTETS_CONTROLLED:
        flowStats.octetsControlled_ref() = value;
        break;
      case SAI_MACSEC_FLOW_STAT_OUT_OCTETS_COMMON:
        flowStats.outCommonOctets_ref() = value;
        break;
      case SAI_MACSEC_FLOW_STAT_OUT_PKTS_TOO_LONG:
        flowStats.outTooLongPkts_ref() = value;
        break;
      case SAI_MACSEC_FLOW_STAT_IN_TAGGED_CONTROL_PKTS:
        flowStats.inTaggedControlledPkts_ref() = value;
        break;
      case SAI_MACSEC_FLOW_STAT_IN_PKTS_NO_TAG:
        flowStats.inNoTagPkts_ref() = value;
        break;
      case SAI_MACSEC_FLOW_STAT_IN_PKTS_BAD_TAG:
        flowStats.inBadTagPkts_ref() = value;
        break;
      case SAI_MACSEC_FLOW_STAT_IN_PKTS_NO_SCI:
        flowStats.noSciPkts_ref() = value;
        break;
      case SAI_MACSEC_FLOW_STAT_IN_PKTS_UNKNOWN_SCI:
        flowStats.unknownSciPkts_ref() = value;
        break;
      case SAI_MACSEC_FLOW_STAT_IN_PKTS_OVERRUN:
        flowStats.overrunPkts_ref() = value;
        break;
    }
  }
  return flowStats;
}

mka::MacsecSaStats fillSaStats(
    const folly::F14FastMap<sai_stat_id_t, uint64_t>& counterId2Value,
    sai_macsec_direction_t direction) {
  mka::MacsecSaStats saStats{};
  saStats.directionIngress_ref() = direction == SAI_MACSEC_DIRECTION_INGRESS;
  for (auto counterIdAndValue : counterId2Value) {
    auto [counterId, value] = counterIdAndValue;
    switch (counterId) {
      case SAI_MACSEC_SA_STAT_OCTETS_ENCRYPTED:
        saStats.octetsEncrypted_ref() = value;
        break;
      case SAI_MACSEC_SA_STAT_OCTETS_PROTECTED:
        saStats.octetsProtected_ref() = value;
        break;
      case SAI_MACSEC_SA_STAT_OUT_PKTS_ENCRYPTED:
        saStats.outEncryptedPkts_ref() = value;
        break;
      case SAI_MACSEC_SA_STAT_OUT_PKTS_PROTECTED:
        saStats.outProtectedPkts_ref() = value;
        break;
      case SAI_MACSEC_SA_STAT_IN_PKTS_UNCHECKED:
        saStats.inUncheckedPkts_ref() = value;
        break;
      case SAI_MACSEC_SA_STAT_IN_PKTS_DELAYED:
        saStats.inDelayedPkts_ref() = value;
        break;
      case SAI_MACSEC_SA_STAT_IN_PKTS_LATE:
        saStats.inLatePkts_ref() = value;
        break;
      case SAI_MACSEC_SA_STAT_IN_PKTS_INVALID:
        saStats.inInvalidPkts_ref() = value;
        break;
      case SAI_MACSEC_SA_STAT_IN_PKTS_NOT_VALID:
        saStats.inNotValidPkts_ref() = value;
        break;
      case SAI_MACSEC_SA_STAT_IN_PKTS_NOT_USING_SA:
        saStats.inNoSaPkts_ref() = value;
        break;
      case SAI_MACSEC_SA_STAT_IN_PKTS_UNUSED_SA:
        saStats.inUnusedSaPkts_ref() = value;
        break;
      case SAI_MACSEC_SA_STAT_IN_PKTS_OK:
        saStats.inOkPkts_ref() = value;
        break;
    }
  }
  return saStats;
}
} // namespace

namespace facebook::fboss {

SaiMacsecManager::SaiMacsecManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable)
    : saiStore_(saiStore), managerTable_(managerTable) {}

SaiMacsecManager::~SaiMacsecManager() {
  for (const auto& macsec : macsecHandles_) {
    auto direction = macsec.first;
    for (const auto& port : macsec.second->ports) {
      auto portId = port.first;

      // Remove the ACL related to SC
      removeScAcls(portId, direction);

      // Delete the default control packet rules in ACL table
      std::string aclTableName = getAclName(portId, direction);
      auto aclTable =
          managerTable_->aclTableManager().getAclTableHandle(aclTableName);
      if (aclTable) {
        // Delete the control packet rules which are created by default for the
        // ACL table LLDP rule
        auto aclEntryHandle =
            managerTable_->aclTableManager().getAclEntryHandle(
                aclTable, kMacsecLldpAclPriority);
        if (aclEntryHandle) {
          auto aclEntry =
              std::make_shared<AclEntry>(kMacsecLldpAclPriority, aclTableName);
          managerTable_->aclTableManager().removeAclEntry(
              aclEntry, aclTableName);
        }

        // MKA rule
        aclEntryHandle = managerTable_->aclTableManager().getAclEntryHandle(
            aclTable, kMacsecMkaAclPriority);
        if (aclEntryHandle) {
          auto aclEntry =
              std::make_shared<AclEntry>(kMacsecMkaAclPriority, aclTableName);
          managerTable_->aclTableManager().removeAclEntry(
              aclEntry, aclTableName);
        }

        // Delete the table
        auto table = std::make_shared<AclTable>(0, aclTableName);
        managerTable_->aclTableManager().removeAclTable(
            table,
            direction == SAI_MACSEC_DIRECTION_INGRESS
                ? cfg::AclStage::INGRESS_MACSEC
                : cfg::AclStage::EGRESS_MACSEC);
      }
    }
  }
}

MacsecSaiId SaiMacsecManager::addMacsec(
    sai_macsec_direction_t direction,
    bool physicalBypassEnable) {
  auto handle = getMacsecHandle(direction);
  if (handle) {
    throw FbossError(
        "Attempted to add macsec for direction that already has a macsec pipeline: ",
        direction,
        " SAI id: ",
        handle->macsec->adapterKey());
  }

  SaiMacsecTraits::CreateAttributes attributes = {
      direction, physicalBypassEnable};
  SaiMacsecTraits::AdapterHostKey key{direction};

  auto& macsecStore = saiStore_->get<SaiMacsecTraits>();
  auto saiMacsecObj = macsecStore.setObject(key, attributes);
  auto macsecHandle = std::make_unique<SaiMacsecHandle>();
  macsecHandle->macsec = std::move(saiMacsecObj);
  macsecHandles_.emplace(direction, std::move(macsecHandle));
  return macsecHandles_[direction]->macsec->adapterKey();
}

void SaiMacsecManager::removeMacsec(sai_macsec_direction_t direction) {
  auto itr = macsecHandles_.find(direction);
  if (itr == macsecHandles_.end()) {
    throw FbossError(
        "Attempted to remove non-existent macsec pipeline for direction: ",
        direction);
  }
  macsecHandles_.erase(itr);
  XLOG(DBG2) << "removed macsec pipeline for direction " << direction;
}

const SaiMacsecHandle* FOLLY_NULLABLE
SaiMacsecManager::getMacsecHandle(sai_macsec_direction_t direction) const {
  return getMacsecHandleImpl(direction);
}
SaiMacsecHandle* FOLLY_NULLABLE
SaiMacsecManager::getMacsecHandle(sai_macsec_direction_t direction) {
  return getMacsecHandleImpl(direction);
}

SaiMacsecHandle* FOLLY_NULLABLE
SaiMacsecManager::getMacsecHandleImpl(sai_macsec_direction_t direction) const {
  auto itr = macsecHandles_.find(direction);
  if (itr == macsecHandles_.end()) {
    return nullptr;
  }
  if (!itr->second.get()) {
    XLOG(FATAL) << "Invalid null SaiMacsecHandle for direction " << direction;
  }
  return itr->second.get();
}

std::shared_ptr<SaiMacsecFlow> SaiMacsecManager::createMacsecFlow(
    sai_macsec_direction_t direction) {
  SaiMacsecFlowTraits::CreateAttributes attributes = {
      direction,
  };
  SaiMacsecFlowTraits::AdapterHostKey key{direction};

  auto& store = saiStore_->get<SaiMacsecFlowTraits>();

  return store.setObject(key, attributes);
}

const SaiMacsecFlow* SaiMacsecManager::getMacsecFlow(
    PortID linePort,
    MacsecSecureChannelId secureChannelId,
    sai_macsec_direction_t direction) const {
  return getMacsecFlowImpl(linePort, secureChannelId, direction);
}
SaiMacsecFlow* SaiMacsecManager::getMacsecFlow(
    PortID linePort,
    MacsecSecureChannelId secureChannelId,
    sai_macsec_direction_t direction) {
  return getMacsecFlowImpl(linePort, secureChannelId, direction);
}

SaiMacsecFlow* SaiMacsecManager::getMacsecFlowImpl(
    PortID linePort,
    MacsecSecureChannelId secureChannelId,
    sai_macsec_direction_t direction) const {
  auto macsecScHandle =
      getMacsecSecureChannelHandle(linePort, secureChannelId, direction);
  if (!macsecScHandle) {
    throw FbossError(
        "Attempted to get macsecFlow for non-existent SC ",
        secureChannelId,
        "for port ",
        linePort,
        " direction ",
        direction);
  }
  return macsecScHandle->flow.get();
}

MacsecPortSaiId SaiMacsecManager::addMacsecPort(
    PortID linePort,
    sai_macsec_direction_t direction) {
  auto macsecPort = getMacsecPortHandle(linePort, direction);
  if (macsecPort) {
    throw FbossError(
        "Attempted to add macsecPort for port/direction that already exists: ",
        linePort,
        ",",
        direction,
        " SAI id: ",
        macsecPort->port->adapterKey());
  }
  auto macsecHandle = getMacsecHandle(direction);
  auto portHandle = managerTable_->portManager().getPortHandle(linePort);
  if (!portHandle) {
    throw FbossError(
        "Attempted to add macsecPort for linePort that doesn't exist: ",
        linePort);
  }

  SaiMacsecPortTraits::CreateAttributes attributes{
      portHandle->port->adapterKey(), direction};
  SaiMacsecPortTraits::AdapterHostKey key{
      portHandle->port->adapterKey(), direction};

  auto& store = saiStore_->get<SaiMacsecPortTraits>();
  auto saiObj = store.setObject(key, attributes);
  auto macsecPortHandle = std::make_unique<SaiMacsecPortHandle>();
  macsecPortHandle->port = std::move(saiObj);

  macsecHandle->ports.emplace(linePort, std::move(macsecPortHandle));
  return macsecHandle->ports[linePort]->port->adapterKey();
}

const SaiMacsecPortHandle* FOLLY_NULLABLE SaiMacsecManager::getMacsecPortHandle(
    PortID linePort,
    sai_macsec_direction_t direction) const {
  return getMacsecPortHandleImpl(linePort, direction);
}
SaiMacsecPortHandle* FOLLY_NULLABLE SaiMacsecManager::getMacsecPortHandle(
    PortID linePort,
    sai_macsec_direction_t direction) {
  return getMacsecPortHandleImpl(linePort, direction);
}

void SaiMacsecManager::removeMacsecPort(
    PortID linePort,
    sai_macsec_direction_t direction) {
  auto macsecHandle = getMacsecHandle(direction);
  if (!macsecHandle) {
    throw FbossError(
        "Attempted to remove macsecPort for direction that has no macsec pipeline obj ",
        direction);
  }
  auto itr = macsecHandle->ports.find(linePort);
  if (itr == macsecHandle->ports.end()) {
    throw FbossError(
        "Attempted to remove non-existent macsec port for lineport, direction: ",
        linePort,
        ", ",
        direction);
  }
  macsecHandle->ports.erase(itr);
  XLOG(DBG2) << "removed macsec port for lineport, direction: " << linePort
             << ", " << direction;
}

SaiMacsecPortHandle* FOLLY_NULLABLE SaiMacsecManager::getMacsecPortHandleImpl(
    PortID linePort,
    sai_macsec_direction_t direction) const {
  auto macsecHandle = getMacsecHandle(direction);
  if (!macsecHandle) {
    throw FbossError(
        "Attempted to get macsecPort for direction that has no macsec pipeline obj ",
        direction);
  }

  auto itr = macsecHandle->ports.find(linePort);
  if (itr == macsecHandle->ports.end()) {
    return nullptr;
  }

  CHECK(itr->second.get())
      << "Invalid null SaiMacsecPortHandle for linePort, direction: "
      << linePort << ", " << direction;

  return itr->second.get();
}

MacsecSCSaiId SaiMacsecManager::addMacsecSecureChannel(
    PortID linePort,
    sai_macsec_direction_t direction,
    MacsecSecureChannelId secureChannelId,
    bool xpn64Enable) {
  auto handle =
      getMacsecSecureChannelHandle(linePort, secureChannelId, direction);
  if (handle) {
    throw FbossError(
        "Attempted to add macsecSC for secureChannelId:direction that already exists: ",
        secureChannelId,
        ":",
        direction,
        " SAI id: ",
        handle->secureChannel->adapterKey());
  }

  auto macsecPort = getMacsecPortHandle(linePort, direction);
  if (!macsecPort) {
    throw FbossError(
        "Attempted to add macsecSC for linePort that doesn't exist: ",
        linePort);
  }

  // create a new flow for the SC
  auto flow = createMacsecFlow(direction);
  auto flowId = flow->adapterKey();

  XLOG(DBG2) << "For ScId " << secureChannelId << " Created Flow " << flowId;

  std::optional<bool> secureChannelEnable;
  std::optional<sai_int32_t> replayProtectionWindow;

  secureChannelEnable = true;

  if (direction == SAI_MACSEC_DIRECTION_INGRESS) {
    replayProtectionWindow = kReplayProtectionWindow;
  }

  SaiMacsecSCTraits::CreateAttributes attributes {
    secureChannelId, direction, flowId,
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
        xpn64Enable ? SAI_MACSEC_CIPHER_SUITE_GCM_AES_XPN_256
                    : SAI_MACSEC_CIPHER_SUITE_GCM_AES_256, /* ciphersuite */
#endif
        secureChannelEnable, replayProtectionWindow.has_value(),
        replayProtectionWindow, kSectagOffset
  };
  SaiMacsecSCTraits::AdapterHostKey secureChannelKey{
      secureChannelId, direction};

  auto& store = saiStore_->get<SaiMacsecSCTraits>();
  auto saiObj = store.setObject(secureChannelKey, attributes);
  auto scHandle = std::make_unique<SaiMacsecSecureChannelHandle>();
  scHandle->secureChannel = std::move(saiObj);
  scHandle->flow = std::move(flow);
  macsecPort->secureChannels.emplace(secureChannelId, std::move(scHandle));
  return macsecPort->secureChannels[secureChannelId]
      ->secureChannel->adapterKey();
}

const SaiMacsecSecureChannelHandle* FOLLY_NULLABLE
SaiMacsecManager::getMacsecSecureChannelHandle(
    PortID linePort,
    MacsecSecureChannelId secureChannelId,
    sai_macsec_direction_t direction) const {
  return getMacsecSecureChannelHandleImpl(linePort, secureChannelId, direction);
}
SaiMacsecSecureChannelHandle* FOLLY_NULLABLE
SaiMacsecManager::getMacsecSecureChannelHandle(
    PortID linePort,
    MacsecSecureChannelId secureChannelId,
    sai_macsec_direction_t direction) {
  return getMacsecSecureChannelHandleImpl(linePort, secureChannelId, direction);
}

void SaiMacsecManager::removeMacsecSecureChannel(
    PortID linePort,
    MacsecSecureChannelId secureChannelId,
    sai_macsec_direction_t direction) {
  auto portHandle = getMacsecPortHandle(linePort, direction);
  if (!portHandle) {
    throw FbossError(
        "Attempted to remove SC for non-existent linePort: ", linePort);
  }
  auto itr = portHandle->secureChannels.find(secureChannelId);
  if (itr == portHandle->secureChannels.end()) {
    throw FbossError(
        "Attempted to remove non-existent macsec SC for linePort:secureChannelId:direction: ",
        linePort,
        ":",
        secureChannelId,
        ":",
        direction);
  }

  // The objects needs to be removed in this order because of the dependency.
  // ACL entry contans reference to Flow
  // SC contains reference to Flow
  // So first we need to remove ACL entry, then Flow and then SC

  // Remove the ACL entry for this SC
  removeScAcls(linePort, direction);
  // Remove the SC
  portHandle->secureChannels[secureChannelId]->secureChannel.reset();
  // No object refers to Flow so it is safe to remove it now
  portHandle->secureChannels[secureChannelId]->flow.reset();
  // Remove the SC from secureChannel map
  portHandle->secureChannels.erase(itr);

  XLOG(INFO) << "removed macsec SC for linePort:secureChannelId:direction: "
             << linePort << ":" << secureChannelId << ":" << direction;
}

SaiMacsecSecureChannelHandle* FOLLY_NULLABLE
SaiMacsecManager::getMacsecSecureChannelHandleImpl(
    PortID linePort,
    MacsecSecureChannelId secureChannelId,
    sai_macsec_direction_t direction) const {
  auto portHandle = getMacsecPortHandle(linePort, direction);
  if (!portHandle) {
    return nullptr;
  }
  auto itr = portHandle->secureChannels.find(secureChannelId);
  if (itr == portHandle->secureChannels.end()) {
    return nullptr;
  }

  CHECK(itr->second.get())
      << "Invalid null SaiMacsecSCHandle for secureChannelId:direction: "
      << secureChannelId << ":" << direction;
  return itr->second.get();
}

MacsecSASaiId SaiMacsecManager::addMacsecSecureAssoc(
    PortID linePort,
    MacsecSecureChannelId secureChannelId,
    sai_macsec_direction_t direction,
    uint8_t assocNum,
    SaiMacsecSak secureAssociationKey,
    SaiMacsecSalt salt,
    SaiMacsecAuthKey authKey,
    MacsecShortSecureChannelId shortSecureChannelId) {
  auto handle =
      getMacsecSecureAssoc(linePort, secureChannelId, direction, assocNum);
  if (handle) {
    throw FbossError(
        "Attempted to add macsecSA for secureChannelId:assocNum that already exists: ",
        secureChannelId,
        ":",
        assocNum,
        " SAI id: ",
        handle->adapterKey());
  }
  auto scHandle =
      getMacsecSecureChannelHandle(linePort, secureChannelId, direction);
  if (!scHandle) {
    throw FbossError(
        "Attempted to add macsecSA for non-existent sc for lineport, secureChannelId, direction: ",
        secureChannelId,
        ", ",
        linePort,
        ", ",
        " direction: ");
  }

  SaiMacsecSATraits::CreateAttributes attributes {
    scHandle->secureChannel->adapterKey(), assocNum, authKey, direction,
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
        shortSecureChannelId,
#endif
        secureAssociationKey, salt, std::nullopt /* minimumXpn */
  };
  SaiMacsecSATraits::AdapterHostKey key{
      scHandle->secureChannel->adapterKey(), assocNum, direction};

  auto& store = saiStore_->get<SaiMacsecSATraits>();
  auto saiObj = store.setObject(key, attributes);

  scHandle->secureAssocs.emplace(assocNum, std::move(saiObj));
  return scHandle->secureAssocs[assocNum]->adapterKey();
}

const SaiMacsecSecureAssoc* FOLLY_NULLABLE
SaiMacsecManager::getMacsecSecureAssoc(
    PortID linePort,
    MacsecSecureChannelId secureChannelId,
    sai_macsec_direction_t direction,
    uint8_t assocNum) const {
  return getMacsecSecureAssocImpl(
      linePort, secureChannelId, direction, assocNum);
}
SaiMacsecSecureAssoc* FOLLY_NULLABLE SaiMacsecManager::getMacsecSecureAssoc(
    PortID linePort,
    MacsecSecureChannelId secureChannelId,
    sai_macsec_direction_t direction,
    uint8_t assocNum) {
  return getMacsecSecureAssocImpl(
      linePort, secureChannelId, direction, assocNum);
}

void SaiMacsecManager::removeMacsecSecureAssoc(
    PortID linePort,
    MacsecSecureChannelId secureChannelId,
    sai_macsec_direction_t direction,
    uint8_t assocNum) {
  auto scHandle =
      getMacsecSecureChannelHandle(linePort, secureChannelId, direction);
  if (!scHandle) {
    throw FbossError(
        "Attempted to remove SA for non-existent secureChannelId: ",
        secureChannelId);
  }
  auto saHandle = scHandle->secureAssocs.find(assocNum);
  if (saHandle == scHandle->secureAssocs.end()) {
    throw FbossError(
        "Attempted to remove non-existent SA for secureChannelId, assocNum:  ",
        secureChannelId,
        ", ",
        assocNum);
  }
  scHandle->secureAssocs.erase(saHandle);
  XLOG(DBG2) << "removed macsec SA for secureChannelId:assocNum: "
             << secureChannelId << ":" << assocNum;
}

SaiMacsecSecureAssoc* FOLLY_NULLABLE SaiMacsecManager::getMacsecSecureAssocImpl(
    PortID linePort,
    MacsecSecureChannelId secureChannelId,
    sai_macsec_direction_t direction,
    uint8_t assocNum) const {
  auto scHandle =
      getMacsecSecureChannelHandle(linePort, secureChannelId, direction);
  if (!scHandle) {
    return nullptr;
  }
  auto itr = scHandle->secureAssocs.find(assocNum);
  if (itr == scHandle->secureAssocs.end()) {
    return nullptr;
  }
  CHECK(itr->second.get())
      << "Invalid null std::shared_ptr<SaiMacsecSA> for secureChannelId:assocNum: "
      << secureChannelId << ":" << assocNum;
  return itr->second.get();
}

void SaiMacsecManager::setupMacsec(
    PortID linePort,
    const mka::MKASak& sak,
    const mka::MKASci& sci,
    sai_macsec_direction_t direction) {
  auto portHandle = managerTable_->portManager().getPortHandle(linePort);
  if (!portHandle) {
    throw FbossError(
        "Failed to get portHandle for non-existent linePort: ", linePort);
  }

  // Get the reverse direction also because we need to find the macsec handle
  // of other direction also
  auto reverseDirection =
      (direction == SAI_MACSEC_DIRECTION_INGRESS
           ? SAI_MACSEC_DIRECTION_EGRESS
           : SAI_MACSEC_DIRECTION_INGRESS);

  if (auto dirMacsecHandle = getMacsecHandle(direction)) {
    // If the macsec handle already exist for this direction then check the
    // current bypass state. If current macsec's bypass_enable is True then
    // update the attribute to make it False
    //    otherwise no issue
    auto bypassEnable = GET_OPT_ATTR(
        Macsec, PhysicalBypass, dirMacsecHandle->macsec->attributes());

    if (bypassEnable) {
      dirMacsecHandle->macsec->setOptionalAttribute(
          SaiMacsecTraits::Attributes::PhysicalBypass{false});
      XLOG(DBG2) << "For direction " << direction
                 << ", set macsec pipeline object w/ ID: " << dirMacsecHandle
                 << ", bypass enable as False";
    }
  } else {
    // If the macsec handle does not exist for this direction then create the
    // macsec handle with bypass_enable as False for this direction
    auto macsecId = addMacsec(direction, false);
    XLOG(DBG2) << "For direction " << direction
               << ", created macsec pipeline object w/ ID: " << macsecId
               << ", bypass enable as False";
  }

  // Also check if the reverse direction macsec handle exist. If it does not
  // exist then we need to create reverse direction macsec handle with
  // bypass_enable as True
  if (!getMacsecHandle(reverseDirection)) {
    auto macsecId = addMacsec(reverseDirection, true);
    XLOG(DBG2) << "For direction " << reverseDirection
               << ", created macsec pipeline object w/ ID: " << macsecId
               << ", bypass enable as True";
  }

  // Check if the macsec port exists for lineport
  if (!getMacsecPortHandle(linePort, direction)) {
    // Step1: Create Macsec egress port (it is not present)
    auto macsecPortId = addMacsecPort(linePort, direction);
    XLOG(DBG2) << "For lineport: " << linePort << ", created macsec "
               << direction << " port " << macsecPortId;
  }

  // Create the egress flow and ingress sc
  std::string sciKeyString =
      folly::to<std::string>(*sci.macAddress_ref(), ".", *sci.port_ref());

  // First convert sci mac address string and the port id to a uint64
  folly::MacAddress mac = folly::MacAddress(*sci.macAddress_ref());
  auto scIdentifier = MacsecSecureChannelId(mac.u64NBO() | *sci.port_ref());

  // Step2/3: Create flow and SC if it they do not exist
  auto secureChannelHandle =
      getMacsecSecureChannelHandle(linePort, scIdentifier, direction);
  if (!secureChannelHandle) {
    auto scSaiId =
        addMacsecSecureChannel(linePort, direction, scIdentifier, true);
    XLOG(DBG2) << "For SCI: " << sciKeyString << ", created macsec "
               << direction << " SC " << scSaiId;
    secureChannelHandle =
        getMacsecSecureChannelHandle(linePort, scIdentifier, direction);
  }
  CHECK(secureChannelHandle);
  auto flowId = secureChannelHandle->flow->adapterKey();

  // Step4: Create the Macsec SA now
  // Input macsec key is a string which needs to be converted to 32 bytes
  // array for passing to sai
  if (sak.keyHex_ref()->size() < 32) {
    XLOG(ERR) << "Macsec key can't be lesser than 32 bytes";
    return;
  }
  std::array<uint8_t, 32> key;
  std::copy(sak.keyHex_ref()->begin(), sak.keyHex_ref()->end(), key.data());

  // Input macsec keyid (auth key) is provided in string which needs to be
  // converted to 16 byte array for passing to sai
  if (sak.keyIdHex_ref()->size() < 16) {
    XLOG(ERR) << "Macsec key Id can't be lesser than 16 bytes";
    return;
  }

  std::array<uint8_t, 16> keyId;
  std::copy(
      sak.keyIdHex_ref()->begin(), sak.keyIdHex_ref()->end(), keyId.data());

  auto secureAssoc = getMacsecSecureAssoc(
      linePort, scIdentifier, direction, *sak.assocNum_ref() % 4);
  if (!secureAssoc) {
    auto secureAssocSaiId = addMacsecSecureAssoc(
        linePort,
        scIdentifier,
        direction,
        *sak.assocNum_ref() % 4,
        key,
        kDefaultSaltValue,
        keyId,
        kDefaultSsciValue);
    XLOG(DBG2) << "For SCI: " << sciKeyString << ", created macsec "
               << direction << " SA " << secureAssocSaiId;
  }

  // // Step5: Create the ACL table if it does not exist
  // Right now we're just using one entry per table, so we're using the same
  // format for table and entry name
  std::string aclName = getAclName(linePort, direction);
  auto aclTable = managerTable_->aclTableManager().getAclTableHandle(aclName);
  if (!aclTable) {
    auto table = std::make_shared<AclTable>(
        0, aclName); // TODO(saranicholas): set appropriate table priority
    auto aclTableId = managerTable_->aclTableManager().addAclTable(
        table,
        direction == SAI_MACSEC_DIRECTION_INGRESS
            ? cfg::AclStage::INGRESS_MACSEC
            : cfg::AclStage::EGRESS_MACSEC);
    XLOG(DBG2) << "For linePort: " << linePort << ", created " << direction
               << " ACL table with sai ID " << aclTableId;
    aclTable = managerTable_->aclTableManager().getAclTableHandle(aclName);

    // After creating ACL table, create couple of control packet rules
    // Rule for MKA
    cfg::EtherType ethTypeMka{cfg::EtherType::EAPOL};
    auto aclEntry = createMacsecControlAclEntry(
        kMacsecMkaAclPriority, aclName, std::nullopt /* dstMac */, ethTypeMka);
    auto aclEntryId =
        managerTable_->aclTableManager().addAclEntry(aclEntry, aclName);
    XLOG(DBG2) << "For linePort: " << linePort << ", direction " << direction
               << " ACL entry for MKA " << aclEntryId;

    // Rule for LLDP
    cfg::EtherType ethTypeLldp{cfg::EtherType::LLDP};
    aclEntry = createMacsecControlAclEntry(
        kMacsecLldpAclPriority,
        aclName,
        std::nullopt /* dstMac */,
        ethTypeLldp);
    aclEntryId =
        managerTable_->aclTableManager().addAclEntry(aclEntry, aclName);
    XLOG(DBG2) << "For linePort: " << linePort << ", direction " << direction
               << " ACL entry for LLDP " << aclEntryId;
  }
  CHECK_NOTNULL(aclTable);
  auto aclTableId = aclTable->aclTable->adapterKey();

  // Step6: Create ACL entry (if does not exist)
  auto aclEntryHandle = managerTable_->aclTableManager().getAclEntryHandle(
      aclTable, kMacsecAclPriority);
  if (!aclEntryHandle) {
    if (direction == SAI_MACSEC_DIRECTION_INGRESS) {
      // For the ingress direction create one ACL rule to match MACSEC packets
      // (ether-type = 0x88E5) with Action as MacSec Flow. Create another lower
      // priority ACL rule to match rest of the packets with Action as Drop (if
      // dropUnencrypted is true) otherwise Forward
      cfg::EtherType ethTypeMacsec{cfg::EtherType::MACSEC};
      auto aclEntry = createMacsecAclEntry(
          kMacsecAclPriority,
          aclName,
          flowId,
          std::nullopt /* dstMac */,
          ethTypeMacsec);

      auto aclEntryId =
          managerTable_->aclTableManager().addAclEntry(aclEntry, aclName);
      XLOG(DBG2) << "For SCI: " << sciKeyString << ", created macsec "
                 << direction << " ACL entry " << aclEntryId;

      // Create the default lower priority ACL rule for rest of the data packets
      cfg::MacsecFlowPacketAction action = sak.dropUnencrypted_ref().value()
          ? cfg::MacsecFlowPacketAction::DROP
          : cfg::MacsecFlowPacketAction::FORWARD;
      aclEntry = createMacsecRxDefaultAclEntry(
          kMacsecDefaultAclPriority, aclName, action);

      aclEntryId =
          managerTable_->aclTableManager().addAclEntry(aclEntry, aclName);
      XLOG(DBG2) << "For SCI: " << sciKeyString
                 << ", created macsec default Rx packet rule "
                 << (sak.dropUnencrypted_ref().value() ? "Drop" : "Forward")
                 << " ACL entry " << aclEntryId;
    } else {
      // For egress direction create the ACL rule to match all data packets with
      // Action as MacSec Flow
      auto aclEntry = createMacsecAclEntry(
          kMacsecAclPriority,
          aclName,
          flowId,
          std::nullopt /* dstMac */,
          std::nullopt /* etherType */);

      auto aclEntryId =
          managerTable_->aclTableManager().addAclEntry(aclEntry, aclName);
      XLOG(DBG2) << "For SCI: " << sciKeyString << ", created macsec "
                 << direction << " ACL entry " << aclEntryId;
    }
  }

  // Step7: Bind ACL table to the line port
  if (direction == SAI_MACSEC_DIRECTION_INGRESS) {
    portHandle->port->setOptionalAttribute(
        SaiPortTraits::Attributes::IngressMacSecAcl{aclTableId});
  } else {
    portHandle->port->setOptionalAttribute(
        SaiPortTraits::Attributes::EgressMacSecAcl{aclTableId});
  }
  XLOG(DBG2) << "To linePort " << linePort << ", bound macsec " << direction
             << " ACL table " << aclTableId;
}

std::string SaiMacsecManager::getAclName(
    facebook::fboss::PortID port,
    sai_macsec_direction_t direction) const {
  return folly::to<std::string>(
      "macsec-",
      direction == SAI_MACSEC_DIRECTION_INGRESS ? "ingress" : "egress",
      "-port",
      port);
}

/* Perform the following:
  1. Delete the secure association
  2. If we have no more secure associations on this secure channel:
    * Delete the secure channel
    * Delete the macsec flow for the channel
    * Delete the acl entry for the channel
*/
void SaiMacsecManager::deleteMacsec(
    PortID linePort,
    const mka::MKASak& sak,
    const mka::MKASci& sci,
    sai_macsec_direction_t direction) {
  std::string sciString =
      folly::to<std::string>(*sci.macAddress_ref(), ".", *sci.port_ref());
  // TODO(ccpowers): Break this back out into a helper method
  auto mac = folly::MacAddress(*sci.macAddress_ref());
  auto scIdentifier = MacsecSecureChannelId(mac.u64NBO() | *sci.port_ref());

  auto assocNum = *sak.assocNum_ref() % 4;

  auto secureChannelHandle =
      getMacsecSecureChannelHandle(linePort, scIdentifier, direction);
  if (!secureChannelHandle) {
    throw FbossError(
        "Secure Channel not found when deleting macsec keys for SCI ",
        sciString,
        " for port: ",
        linePort);
  }

  auto secureAssoc =
      getMacsecSecureAssoc(linePort, scIdentifier, direction, assocNum);
  if (!secureAssoc) {
    throw FbossError(
        "Secure Association not found when deleting macsec keys for SCI ",
        sciString,
        "for port: ",
        linePort);
  }

  removeMacsecSecureAssoc(linePort, scIdentifier, direction, assocNum);

  // If there's no SA's on this SC, remove the SC
  if (secureChannelHandle->secureAssocs.empty()) {
    removeMacsecSecureChannel(linePort, scIdentifier, direction);
  }
}

void SaiMacsecManager::removeScAcls(
    PortID linePort,
    sai_macsec_direction_t direction) {
  // unbind acl tables from the port, and delete acl entries
  auto portHandle = managerTable_->portManager().getPortHandle(linePort);

  if (direction == SAI_MACSEC_DIRECTION_INGRESS) {
    portHandle->port->setOptionalAttribute(
        SaiPortTraits::Attributes::IngressMacSecAcl{SAI_NULL_OBJECT_ID});
  } else {
    portHandle->port->setOptionalAttribute(
        SaiPortTraits::Attributes::EgressMacSecAcl{SAI_NULL_OBJECT_ID});
  }
  XLOG(DBG2) << "removeScAcls: Unbound ACL table from line port " << linePort;

  std::string aclName = getAclName(linePort, direction);
  auto aclTable = managerTable_->aclTableManager().getAclTableHandle(aclName);
  if (aclTable) {
    auto aclEntryHandle = managerTable_->aclTableManager().getAclEntryHandle(
        aclTable, kMacsecAclPriority);
    if (aclEntryHandle) {
      auto aclEntry = std::make_shared<AclEntry>(kMacsecAclPriority, aclName);
      managerTable_->aclTableManager().removeAclEntry(aclEntry, aclName);
      XLOG(DBG2) << "removeScAcls: Removed ACL entry from ACL table "
                 << aclName;

      // Remove the entry for default Rx packet rule also
      if (direction == SAI_MACSEC_DIRECTION_INGRESS) {
        aclEntryHandle = managerTable_->aclTableManager().getAclEntryHandle(
            aclTable, kMacsecDefaultAclPriority);
        if (aclEntryHandle) {
          aclEntry =
              std::make_shared<AclEntry>(kMacsecDefaultAclPriority, aclName);
          managerTable_->aclTableManager().removeAclEntry(aclEntry, aclName);
          XLOG(DBG2)
              << "removeScAcls: Removed default packet ACL entry from ACL table "
              << aclName;
        }
      }
    }
  }
}

namespace {
void updateMacsecPortStats(
    const std::optional<mka::MacsecFlowStats>& prev,
    const mka::MacsecFlowStats& cur,
    mka::MacsecPortStats& portStats) {
  *portStats.inBadOrNoMacsecTagDroppedPkts_ref() +=
      (utility::CounterPrevAndCur(
           prev ? *prev->inNoTagPkts_ref() : 0L, *cur.inNoTagPkts_ref())
           .incrementFromPrev() +
       utility::CounterPrevAndCur(
           prev ? *prev->inBadTagPkts_ref() : 0L, *cur.inBadTagPkts_ref())
           .incrementFromPrev());
  *portStats.inNoSciDroppedPkts_ref() +=
      utility::CounterPrevAndCur(
          prev ? *prev->noSciPkts_ref() : 0L, *cur.noSciPkts_ref())
          .incrementFromPrev();
  *portStats.inUnknownSciPkts_ref() +=
      utility::CounterPrevAndCur(
          prev ? *prev->unknownSciPkts_ref() : 0L, *cur.unknownSciPkts_ref())
          .incrementFromPrev();
  *portStats.inOverrunDroppedPkts_ref() +=
      utility::CounterPrevAndCur(
          prev ? *prev->overrunPkts_ref() : 0L, *cur.overrunPkts_ref())
          .incrementFromPrev();

  *portStats.outTooLongDroppedPkts_ref() +=
      utility::CounterPrevAndCur(
          prev ? *prev->outTooLongPkts_ref() : 0L, *cur.untaggedPkts_ref())
          .incrementFromPrev();
  *portStats.noMacsecTagPkts_ref() +=
      utility::CounterPrevAndCur(
          prev ? *prev->untaggedPkts_ref() : 0L, *cur.untaggedPkts_ref())
          .incrementFromPrev();
}

void updateMacsecPortStats(
    const std::optional<mka::MacsecSaStats>& prev,
    const mka::MacsecSaStats& cur,
    mka::MacsecPortStats& portStats) {
  *portStats.octetsEncrypted_ref() +=
      utility::CounterPrevAndCur(
          prev ? *prev->octetsEncrypted_ref() : 0L, *cur.octetsEncrypted_ref())
          .incrementFromPrev();
  *portStats.inDelayedPkts_ref() +=
      utility::CounterPrevAndCur(
          prev ? *prev->inDelayedPkts_ref() : 0L, *cur.inDelayedPkts_ref())
          .incrementFromPrev();
  *portStats.inLateDroppedPkts_ref() +=
      utility::CounterPrevAndCur(
          prev ? *prev->inLatePkts_ref() : 0L, *cur.inLatePkts_ref())
          .incrementFromPrev();
  *portStats.inNotValidDroppedPkts_ref() +=
      utility::CounterPrevAndCur(
          prev ? *prev->inNotValidPkts_ref() : 0L, *cur.inNotValidPkts_ref())
          .incrementFromPrev();
  *portStats.inInvalidPkts_ref() +=
      utility::CounterPrevAndCur(
          prev ? *prev->inInvalidPkts_ref() : 0L, *cur.inInvalidPkts_ref())
          .incrementFromPrev();
  *portStats.inNoSaDroppedPkts_ref() +=
      utility::CounterPrevAndCur(
          prev ? *prev->inNoSaPkts_ref() : 0L, *cur.inNoSaPkts_ref())
          .incrementFromPrev();
  *portStats.inUnusedSaPkts_ref() +=
      utility::CounterPrevAndCur(
          prev ? *prev->inUnusedSaPkts_ref() : 0L, *cur.inUnusedSaPkts_ref())
          .incrementFromPrev();
}
} // namespace
void SaiMacsecManager::updateStats(PortID port, HwPortStats& portStats) {
  for (const auto& dirAndMacsec : macsecHandles_) {
    const auto& [direction, macsec] = dirAndMacsec;
    auto pitr = macsec->ports.find(port);
    if (pitr == macsec->ports.end()) {
      continue;
    }
    auto& macsecPort = *pitr;
    if (!portStats.macsecStats_ref()) {
      portStats.macsecStats_ref() = MacsecStats{};
    }
    auto& macsecStats = *portStats.macsecStats_ref();
    auto& macsecPortStats = direction == SAI_MACSEC_DIRECTION_INGRESS
        ? *macsecStats.ingressPortStats_ref()
        : *macsecStats.egressPortStats_ref();
    // Record flow, SA stats into a new map. We will then use this to
    // updated macsecStats flow stats map. This will take care of
    // removing any flows that have been deleted since last stat
    // collection
    std::vector<MacsecSciFlowStats> newFlowStats;
    std::vector<MacsecSaIdSaStats> newSaStats;
    auto& flowStats = direction == SAI_MACSEC_DIRECTION_INGRESS
        ? *macsecStats.ingressFlowStats_ref()
        : *macsecStats.egressFlowStats_ref();
    auto& saStats = direction == SAI_MACSEC_DIRECTION_INGRESS
        ? *macsecStats.rxSecureAssociationStats_ref()
        : *macsecStats.txSecureAssociationStats_ref();
    for (const auto& macsecSc : macsecPort.second->secureChannels) {
      MacsecSecureChannelId secureChannelId = macsecSc.first;
      mka::MKASci sci;
      sci.macAddress_ref() =
          folly::MacAddress::fromNBO((secureChannelId >> 16) << 16).toString();
      sci.port_ref() = secureChannelId & 0xFF;
      for (const auto& macsecSa : macsecSc.second->secureAssocs) {
        macsecSa.second->updateStats<SaiMacsecSATraits>();
        mka::MKASecureAssociationId saId;
        saId.sci_ref() = sci;
        saId.assocNum_ref() = macsecSa.first;

        // Find saStats for a given sa
        auto findSaStats = [](const std::vector<MacsecSaIdSaStats>& saStats,
                              mka::MKASecureAssociationId sa)
            -> std::optional<mka::MacsecSaStats> {
          for (const auto& saStat : saStats) {
            if (saStat.saId_ref().value() == sa) {
              return saStat.saStats_ref().value();
            }
          }
          return std::nullopt;
        };

        auto prevSaStats = findSaStats(saStats, saId);
        auto curSaStats = fillSaStats(macsecSa.second->getStats(), direction);
        updateMacsecPortStats(prevSaStats, curSaStats, macsecPortStats);

        MacsecSaIdSaStats singleSaStats;
        singleSaStats.saId_ref() = saId;
        singleSaStats.saStats_ref() = curSaStats;

        newSaStats.push_back(singleSaStats);
      }
      macsecSc.second->flow->updateStats<SaiMacsecFlowTraits>();
      auto curFlowStats =
          fillFlowStats(macsecSc.second->flow->getStats(), direction);

      // Find flowStats for a given sci
      auto findFlowStats =
          [](const std::vector<MacsecSciFlowStats>& flowStats,
             mka::MKASci sci) -> std::optional<mka::MacsecFlowStats> {
        for (const auto& flowStat : flowStats) {
          if (flowStat.sci_ref().value() == sci) {
            return flowStat.flowStats_ref().value();
          }
        }
        return std::nullopt;
      };

      auto prevFlowStats = findFlowStats(flowStats, sci);
      updateMacsecPortStats(prevFlowStats, curFlowStats, macsecPortStats);

      MacsecSciFlowStats singleFlowStats;
      singleFlowStats.sci_ref() = sci;
      singleFlowStats.flowStats_ref() = curFlowStats;
      newFlowStats.push_back(singleFlowStats);
    }
    flowStats = std::move(newFlowStats);
    saStats = std::move(newSaStats);
    // Port stats
    macsecPort.second->port->updateStats<SaiMacsecPortTraits>();
    fillHwPortStats(macsecPort.second->port->getStats(), macsecPortStats);

    // ACL counters for default rule on ingress
    if (direction == SAI_MACSEC_DIRECTION_INGRESS) {
      std::string aclTblName = getAclName(port, direction);
      auto aclTable =
          managerTable_->aclTableManager().getAclTableHandle(aclTblName);
      if (aclTable) {
        // Lambda to get the ACL counter value
        auto getAclCounter = [this, &aclTable](int prio) -> uint64_t {
          auto aclEntryHandle =
              managerTable_->aclTableManager().getAclEntryHandle(
                  aclTable, prio);
          if (aclEntryHandle) {
            auto aclEntryId = aclEntryHandle->aclEntry->adapterKey();
            auto aclCounterIdGot =
                SaiApiTable::getInstance()
                    ->aclApi()
                    .getAttribute(
                        AclEntrySaiId(aclEntryId),
                        SaiAclEntryTraits::Attributes::ActionCounter())
                    .getData();
            auto counterPackets =
                SaiApiTable::getInstance()->aclApi().getAttribute(
                    AclCounterSaiId(aclCounterIdGot),
                    SaiAclCounterTraits::Attributes::CounterPackets());
            return counterPackets;
          }
          XLOG(ERR) << folly::sformat(
              "ACL entry does not exist for priority {:d}", prio);
          return 0;
        };
        auto defaultAclCount = getAclCounter(kMacsecDefaultAclPriority);
        XLOG(DBG5) << folly::sformat(
            "ACL counter Macsec default = {:d}", defaultAclCount);
        auto& macsecAclStats = *macsecStats.ingressAclStats_ref();
        macsecAclStats.defaultAclStats_ref() = defaultAclCount;
      }
    }
  }
}
} // namespace facebook::fboss
