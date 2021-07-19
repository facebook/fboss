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
constexpr int kMacsecAclPriority = 1;

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
  macsecFlowAction.flowId_ref() = flowId;
  macsecAction.setMacsecFlow(macsecFlowAction);
  entry->setActionType(cfg::AclActionType::PERMIT);
  entry->setAclAction(macsecAction);

  return entry;
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
      // unbind acl tables from the port, and delete acl entries, since they
      // reference our macsecport/flow objects that we're trying to destroy
      auto portId = port.first;
      auto portHandle = managerTable_->portManager().getPortHandle(portId);
      portHandle->port->setOptionalAttribute(
          SaiPortTraits::Attributes::IngressMacSecAcl{SAI_NULL_OBJECT_ID});
      portHandle->port->setOptionalAttribute(
          SaiPortTraits::Attributes::EgressMacSecAcl{SAI_NULL_OBJECT_ID});

      std::string aclName = getAclName(portId, direction);
      auto aclTable =
          managerTable_->aclTableManager().getAclTableHandle(aclName);
      if (aclTable) {
        auto aclEntryHandle =
            managerTable_->aclTableManager().getAclEntryHandle(
                aclTable, kMacsecAclPriority);
        if (aclEntryHandle) {
          auto aclEntry =
              std::make_shared<AclEntry>(kMacsecAclPriority, aclName);
          managerTable_->aclTableManager().removeAclEntry(aclEntry, aclName);
        }
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

  std::optional<bool> secureChannelEnable;
  std::optional<sai_int32_t> replayProtectionWindow;

  if (direction == SAI_MACSEC_DIRECTION_EGRESS) {
    secureChannelEnable = true;
  }
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
  portHandle->secureChannels.erase(itr);
  XLOG(DBG2) << "removed macsec SC for linePort:secureChannelId:direction: "
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

  // Create macsec pipeline obj if it doesn't exist
  if (!getMacsecHandle(SAI_MACSEC_DIRECTION_INGRESS)) {
    auto macsecId =
        addMacsec(SAI_MACSEC_DIRECTION_INGRESS, false /* phys bypass enable */);
    XLOG(DBG2) << "For direction " << SAI_MACSEC_DIRECTION_INGRESS
               << ", created macsec pipeline object w/ ID: " << macsecId;
  }
  if (!getMacsecHandle(SAI_MACSEC_DIRECTION_EGRESS)) {
    auto macsecId =
        addMacsec(SAI_MACSEC_DIRECTION_EGRESS, false /* phys bypass enable */);
    XLOG(DBG2) << "For direction " << SAI_MACSEC_DIRECTION_EGRESS
               << ", created macsec pipeline object w/ ID: " << macsecId;
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
               << direction << " SA " << sciKeyString << secureAssocSaiId;
  }

  // // Step5: Create the ACL table if it does not exist
  // Right now we're just using one entry per table, so we're using the same
  // format for table and entry name
  std::string aclName = getAclName(linePort, direction);
  auto aclTable = managerTable_->aclTableManager().getAclTableHandle(aclName);
  if (!aclTable) {
    auto aclTableId = managerTable_->aclTableManager().addAclTable(
        aclName,
        direction == SAI_MACSEC_DIRECTION_INGRESS
            ? SAI_ACL_STAGE_INGRESS_MACSEC
            : SAI_ACL_STAGE_EGRESS_MACSEC);
    XLOG(DBG2) << "For linePort: " << linePort << ", created " << direction
               << " ACL table with sai ID " << aclTableId;
    aclTable = managerTable_->aclTableManager().getAclTableHandle(aclName);
  }
  CHECK_NOTNULL(aclTable);
  auto aclTableId = aclTable->aclTable->adapterKey();

  // Step6: Create ACL entry (if does not exist)
  auto aclEntryHandle = managerTable_->aclTableManager().getAclEntryHandle(
      aclTable, kMacsecAclPriority);
  if (!aclEntryHandle) {
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
    sai_macsec_direction_t direction) {
  return folly::to<std::string>(
      "macsec-",
      direction == SAI_MACSEC_DIRECTION_INGRESS ? "ingress" : "egress",
      "-port",
      port);
}

} // namespace facebook::fboss
