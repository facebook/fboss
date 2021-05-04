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

#include "fboss/agent/hw/sai/api/MacsecApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"

#include <folly/CppAttributes.h>
#include "folly/container/F14Map.h"

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;

using SaiMacsec = SaiObject<SaiMacsecTraits>;
using SaiMacsecPort = SaiObject<SaiMacsecPortTraits>;
using SaiMacsecSA = SaiObject<SaiMacsecSATraits>;
using SaiMacsecSC = SaiObject<SaiMacsecSCTraits>;
using SaiMacsecFlow = SaiObject<SaiMacsecFlowTraits>;

struct SaiMacsecSCHandle {
  std::shared_ptr<SaiMacsecSC> sc;
  folly::F14FastMap<uint8_t, std::shared_ptr<SaiMacsecSA>> secureAssocs;
};

struct SaiMacsecPortHandle {
  std::shared_ptr<SaiMacsecPort> port;
  folly::F14FastMap<MacsecSecureChannelId, std::unique_ptr<SaiMacsecSCHandle>>
      secureChannels;
};

struct SaiMacsecHandle {
  std::shared_ptr<SaiMacsec> macsec;
  std::shared_ptr<SaiMacsecFlow> flow;
  folly::F14FastMap<PortID, std::unique_ptr<SaiMacsecPortHandle>> ports;
};

class SaiMacsecManager {
  using MacsecHandles = folly::
      F14FastMap<sai_macsec_direction_t, std::unique_ptr<SaiMacsecHandle>>;

 public:
  explicit SaiMacsecManager(SaiStore* saiStore);
  ~SaiMacsecManager();

  SaiMacsecHandle* FOLLY_NULLABLE
  getMacsecHandle(sai_macsec_direction_t direction);
  const SaiMacsecHandle* FOLLY_NULLABLE
  getMacsecHandle(sai_macsec_direction_t direction) const;
  void removeMacsec(sai_macsec_direction_t direction);

 private:
  MacsecSaiId addMacsec(
      sai_macsec_direction_t direction,
      bool physicalBypassEnable);
  SaiMacsecHandle* FOLLY_NULLABLE
  getMacsecHandleImpl(sai_macsec_direction_t direction) const;

  MacsecFlowSaiId addMacsecFlow(sai_macsec_direction_t direction);
  const SaiMacsecFlow* getMacsecFlow(sai_macsec_direction_t direction) const;
  SaiMacsecFlow* getMacsecFlow(sai_macsec_direction_t direction);
  SaiMacsecFlow* getMacsecFlowImpl(sai_macsec_direction_t direction) const;
  void removeMacsecFlow(sai_macsec_direction_t direction);

  SaiStore* saiStore_;

  MacsecHandles macsecHandles_;
};

} // namespace facebook::fboss
