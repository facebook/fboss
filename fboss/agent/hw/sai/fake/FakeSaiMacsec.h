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

#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/api/Types.h"
#include "fboss/agent/hw/sai/fake/FakeManager.h"

extern "C" {
#include <sai.h>
#include "fboss/agent/hw/sai/api/fake/saifakeextensions.h"
}

namespace facebook::fboss {

struct FakeMacsec {
  explicit FakeMacsec(sai_macsec_direction_t _direction)
      : direction(_direction) {}
  sai_object_id_t id;
  sai_macsec_direction_t direction;
  bool physicalBypass{false};
};

struct FakeMacsecPort {
  explicit FakeMacsecPort(
      sai_object_id_t _portID,
      sai_macsec_direction_t _macsecDirection)
      : portID(_portID), macsecDirection(_macsecDirection) {}
  sai_object_id_t id;
  sai_object_id_t portID;
  sai_macsec_direction_t macsecDirection;
};

struct FakeMacsecSA {
  explicit FakeMacsecSA(
      sai_object_id_t _scid,
      sai_uint8_t _an,
      const sai_macsec_auth_key_t& _authKey,
      sai_macsec_direction_t _macsecDirection,
      sai_uint32_t _ssci,
      const sai_macsec_sak_t& _sak,
      const sai_macsec_salt_t& _salt)
      : scid(_scid), an(_an), macsecDirection(_macsecDirection), ssci(_ssci) {
    std::copy(std::begin(_authKey), std::end(_authKey), std::begin(authKey));
    std::copy(std::begin(_sak), std::end(_sak), std::begin(sak));
    std::copy(std::begin(_salt), std::end(_salt), std::begin(salt));
  }
  sai_object_id_t id;
  sai_object_id_t scid;
  sai_uint8_t an;
  SaiMacsecAuthKey authKey;
  sai_macsec_direction_t macsecDirection;
  sai_uint32_t ssci;
  SaiMacsecSak sak;
  SaiMacsecSalt salt;
  sai_uint64_t minimumXpn{0};
  sai_uint64_t currentXpn{0};
};

struct FakeMacsecSC {
  explicit FakeMacsecSC(
      sai_uint64_t _sci,
      sai_macsec_direction_t _macsecDirection,
      sai_object_id_t _flowId
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
      ,
      sai_macsec_cipher_suite_t _cipherSuite
#endif
      )
      : sci(_sci),
        macsecDirection(_macsecDirection),
        flowId(_flowId)
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
        ,
        cipherSuite(_cipherSuite)
#endif
  {
  }
  sai_object_id_t id;
  sai_uint64_t sci;
  sai_macsec_direction_t macsecDirection;
  sai_object_id_t activeEgressSAID{0};
  sai_object_id_t flowId;
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
  sai_macsec_cipher_suite_t cipherSuite;
#endif
  bool sciEnable{false};
  bool replayProtectionEnable{false};
  sai_int32_t replayProtectionWindow{0};
  sai_uint8_t sectagOffset{0};
};

struct FakeMacsecFlow {
  explicit FakeMacsecFlow(sai_macsec_direction_t _macsecDirection)
      : macsecDirection(_macsecDirection) {}
  sai_object_id_t id;
  sai_macsec_direction_t macsecDirection;
};

using FakeMacsecManager = FakeManager<sai_object_id_t, FakeMacsec>;
using FakeMacsecPortManager = FakeManager<sai_object_id_t, FakeMacsecPort>;
using FakeMacsecSAManager = FakeManager<sai_object_id_t, FakeMacsecSA>;
using FakeMacsecSCManager = FakeManager<sai_object_id_t, FakeMacsecSC>;
using FakeMacsecFlowManager = FakeManager<sai_object_id_t, FakeMacsecFlow>;

void populate_macsec_api(sai_macsec_api_t** macsec_api);

} // namespace facebook::fboss
