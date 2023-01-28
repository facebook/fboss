/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <fboss/mka_service/if/gen-cpp2/mka_structs_types.h>
#include "fboss/agent/hw/sai/api/MacsecApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiObjectWithCounters.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"

#include <folly/CppAttributes.h>
#include <gtest/gtest.h>
#include "folly/container/F14Map.h"

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class HwPortStats;

using SaiMacsec = SaiObject<SaiMacsecTraits>;
using SaiMacsecPort = SaiObjectWithCounters<SaiMacsecPortTraits>;
using SaiMacsecSecureAssoc = SaiObjectWithCounters<SaiMacsecSATraits>;
using SaiMacsecSecureChannel = SaiObject<SaiMacsecSCTraits>;
using SaiMacsecFlow = SaiObjectWithCounters<SaiMacsecFlowTraits>;

struct SaiMacsecSecureChannelHandle {
  // Flow must come before SC for destruction to remove them in the right order
  std::shared_ptr<SaiMacsecFlow> flow;
  std::shared_ptr<SaiMacsecSecureChannel> secureChannel;
  folly::F14FastMap<uint8_t, std::shared_ptr<SaiMacsecSecureAssoc>>
      secureAssocs;
  uint8_t latestSaAn{0};
};

struct SaiMacsecPortHandle {
  std::shared_ptr<SaiMacsecPort> port;
  // map from SCI (mac address + port ID) to secureChannel
  folly::F14FastMap<
      MacsecSecureChannelId,
      std::unique_ptr<SaiMacsecSecureChannelHandle>>
      secureChannels;
};

struct SaiMacsecHandle {
  std::shared_ptr<SaiMacsec> macsec;
  folly::F14FastMap<PortID, std::unique_ptr<SaiMacsecPortHandle>> ports;
};

class SaiMacsecManager {
  using MacsecHandles = folly::
      F14FastMap<sai_macsec_direction_t, std::unique_ptr<SaiMacsecHandle>>;

 public:
  SaiMacsecManager(SaiStore* saiStore, SaiManagerTable* managerTable);
  ~SaiMacsecManager();

  void setupMacsec(
      PortID linePort,
      const mka::MKASak& sak,
      const mka::MKASci& sci,
      sai_macsec_direction_t direction);

  void deleteMacsec(
      PortID linePort,
      const mka::MKASak& sak,
      const mka::MKASci& sci,
      sai_macsec_direction_t direction,
      bool skipHwUpdate = false);

  void updateStats(PortID port, HwPortStats& portStats);

  void
  setMacsecState(PortID linePort, bool macsecDesired, bool dropUnencrypted);

  std::optional<MacsecSASaiId> getMacsecSaAdapterKey(
      PortID linePort,
      sai_macsec_direction_t direction,
      const mka::MKASci& sci,
      uint8_t assocNum);

 private:
  const SaiMacsecHandle* FOLLY_NULLABLE
  getMacsecHandle(sai_macsec_direction_t direction) const;

  const SaiMacsecFlow* getMacsecFlow(
      PortID linePort,
      MacsecSecureChannelId secureChannelId,
      sai_macsec_direction_t direction) const;

  const SaiMacsecPortHandle* FOLLY_NULLABLE
  getMacsecPortHandle(PortID linePort, sai_macsec_direction_t direction) const;

  const SaiMacsecSecureChannelHandle* FOLLY_NULLABLE
  getMacsecSecureChannelHandle(
      PortID linePort,
      MacsecSecureChannelId secureChannelId,
      sai_macsec_direction_t direction) const;

  const SaiMacsecSecureAssoc* FOLLY_NULLABLE getMacsecSecureAssoc(
      PortID linePort,
      MacsecSecureChannelId secureChannelId,
      sai_macsec_direction_t direction,
      uint8_t assocNum) const;
  std::string getAclName(
      facebook::fboss::PortID port,
      sai_macsec_direction_t direction) const;

  SaiMacsecHandle* FOLLY_NULLABLE
  getMacsecHandle(sai_macsec_direction_t direction);
  SaiMacsecHandle* FOLLY_NULLABLE
  getMacsecHandleImpl(sai_macsec_direction_t direction) const;

  SaiMacsecFlow* getMacsecFlow(
      PortID linePort,
      MacsecSecureChannelId secureChannelId,
      sai_macsec_direction_t direction);
  SaiMacsecFlow* getMacsecFlowImpl(
      PortID linePort,
      MacsecSecureChannelId secureChannelId,
      sai_macsec_direction_t direction) const;

  SaiMacsecPortHandle* FOLLY_NULLABLE
  getMacsecPortHandle(PortID linePort, sai_macsec_direction_t direction);
  SaiMacsecPortHandle* FOLLY_NULLABLE getMacsecPortHandleImpl(
      PortID linePort,
      sai_macsec_direction_t direction) const;

  SaiMacsecSecureChannelHandle* FOLLY_NULLABLE getMacsecSecureChannelHandle(
      PortID linePort,
      MacsecSecureChannelId secureChannelId,
      sai_macsec_direction_t direction);
  SaiMacsecSecureChannelHandle* FOLLY_NULLABLE getMacsecSecureChannelHandleImpl(
      PortID linePort,
      MacsecSecureChannelId secureChannelId,
      sai_macsec_direction_t direction) const;

  SaiMacsecSecureAssoc* FOLLY_NULLABLE getMacsecSecureAssoc(
      PortID linePort,
      MacsecSecureChannelId secureChannelId,
      sai_macsec_direction_t direction,
      uint8_t assocNum);

  SaiMacsecSecureAssoc* FOLLY_NULLABLE getMacsecSecureAssocImpl(
      PortID linePort,
      MacsecSecureChannelId secureChannelId,
      sai_macsec_direction_t direction,
      uint8_t assocNum) const;
  std::shared_ptr<SaiMacsecFlow> createMacsecFlow(
      sai_macsec_direction_t direction);

  MacsecSaiId addMacsec(
      sai_macsec_direction_t direction,
      bool physicalBypassEnable);
  MacsecPortSaiId addMacsecPort(
      PortID linePort,
      sai_macsec_direction_t direction);
  MacsecSCSaiId addMacsecSecureChannel(
      PortID linePort,
      sai_macsec_direction_t direction,
      MacsecSecureChannelId secureChannelId,
      bool xpn64Enable);
  MacsecSASaiId addMacsecSecureAssoc(
      PortID linePort,
      MacsecSecureChannelId secureChannelId,
      sai_macsec_direction_t direction,
      uint8_t assocNum,
      SaiMacsecSak secureAssociationKey,
      SaiMacsecSalt salt,
      SaiMacsecAuthKey authKey,
      MacsecShortSecureChannelId shortSecureChannelId);

  void removeMacsec(sai_macsec_direction_t direction);
  void removeMacsecPort(PortID linePort, sai_macsec_direction_t direction);
  void removeMacsecSecureChannel(
      PortID linePort,
      MacsecSecureChannelId secureChannelId,
      sai_macsec_direction_t direction);
  void removeMacsecSecureAssoc(
      PortID linePort,
      MacsecSecureChannelId secureChannelId,
      sai_macsec_direction_t direction,
      uint8_t assocNum,
      bool skipHwUpdate = false);

  void removeScAcls(PortID linePort, sai_macsec_direction_t direction);

  void setupMacsecState(
      PortID linePort,
      bool dropUnencrypted,
      sai_macsec_direction_t direction);

  void removeMacsecState(PortID linePort, sai_macsec_direction_t direction);

  void setupMacsecPipeline(PortID linePort, sai_macsec_direction_t direction);

  void setupMacsecPort(PortID linePort, sai_macsec_direction_t direction);

  void setupAclTable(PortID linePort, sai_macsec_direction_t direction);

  void setupAclControlPacketRules(
      PortID linePort,
      sai_macsec_direction_t direction);

  void setupDropUnencryptedRule(
      PortID linePort,
      bool dropUnencrypted,
      sai_macsec_direction_t direction);

  void removeMacsecPipeline(PortID linePort, sai_macsec_direction_t direction);

  void removeMacsecVport(PortID linePort, sai_macsec_direction_t direction);

  void removeAclTable(PortID linePort, sai_macsec_direction_t direction);

  void removeAclControlPacketRules(
      PortID linePort,
      sai_macsec_direction_t direction);

  void removeDropUnencryptedRule(
      PortID linePort,
      sai_macsec_direction_t direction);

  SaiStore* saiStore_;

  MacsecHandles macsecHandles_;

  SaiManagerTable* managerTable_;

  // TODO(ccpowers): It might make sense to just delete any of these tests that
  // can't just be driven through the setupMacsec() interface
  FRIEND_TEST(MacsecManagerTest, addMacsec);
  FRIEND_TEST(MacsecManagerTest, addDuplicateMacsec);
  FRIEND_TEST(MacsecManagerTest, removeMacsec);
  FRIEND_TEST(MacsecManagerTest, removeNonexistentMacsec);
  FRIEND_TEST(MacsecManagerTest, addMacsecPort);
  FRIEND_TEST(MacsecManagerTest, addMacsecPortForNonexistentMacsec);
  FRIEND_TEST(MacsecManagerTest, addMacsecPortForNonexistentPort);
  FRIEND_TEST(MacsecManagerTest, addDuplicateMacsecPort);
  FRIEND_TEST(MacsecManagerTest, removeMacsecPort);
  FRIEND_TEST(MacsecManagerTest, removeNonexistentMacsecPort);
  FRIEND_TEST(MacsecManagerTest, addMacsecSecureChannel);
  FRIEND_TEST(MacsecManagerTest, addMacsecSecureChannelForNonexistentMacsec);
  FRIEND_TEST(
      MacsecManagerTest,
      addMacsecSecureChannelForNonexistentMacsecPort);
  FRIEND_TEST(MacsecManagerTest, addDuplicateMacsecSecureChannel);
  FRIEND_TEST(MacsecManagerTest, removeMacsecSecureChannel);
  FRIEND_TEST(MacsecManagerTest, removeNonexistentMacsecSecureChannel);
  FRIEND_TEST(MacsecManagerTest, addMacsecSecureAssoc);
  FRIEND_TEST(
      MacsecManagerTest,
      addMacsecSecureAssocForNonexistentSecureChannel);
  FRIEND_TEST(MacsecManagerTest, addDuplicateMacsecSecureAssoc);
  FRIEND_TEST(MacsecManagerTest, removeMacsecSecureAssoc);
  FRIEND_TEST(MacsecManagerTest, removeNonexistentMacsecSecureAssoc);
  FRIEND_TEST(MacsecManagerTest, deleteKeysWithOneSecureAssoc);
  FRIEND_TEST(MacsecManagerTest, deleteKeysWithMultipleSecureAssoc);
  FRIEND_TEST(MacsecManagerTest, deleteKeysWithSingleSecureAssoc);
  FRIEND_TEST(MacsecManagerTest, invalidLinePort);
  FRIEND_TEST(MacsecManagerTest, installKeys);
  friend class HwMacsecTest;
};

} // namespace facebook::fboss
