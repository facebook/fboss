/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/fake/FakeSaiPort.h"

#include <folly/logging/xlog.h>
#include <optional>

using facebook::fboss::FakeSai;

sai_status_t create_macsec_fn(
    sai_object_id_t* macsec_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  sai_macsec_direction_t direction;
  std::optional<bool> physicalBypass;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_MACSEC_ATTR_DIRECTION:
        direction = static_cast<sai_macsec_direction_t>(attr_list[i].value.s32);
        break;
      case SAI_MACSEC_ATTR_PHYSICAL_BYPASS_ENABLE:
        physicalBypass = attr_list[i].value.booldata;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }

  *macsec_id = fs->macsecManager.create(direction);
  auto& macsec = fs->macsecManager.get(*macsec_id);
  if (physicalBypass) {
    macsec.physicalBypass = physicalBypass.value();
  }

  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_macsec_fn(sai_object_id_t macsec_id) {
  FakeSai::getInstance()->macsecManager.remove(macsec_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_macsec_attribute_fn(
    sai_object_id_t macsec_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& macsec = fs->macsecManager.get(macsec_id);
  sai_status_t res = SAI_STATUS_SUCCESS;
  if (!attr) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  switch (attr->id) {
    case SAI_MACSEC_ATTR_PHYSICAL_BYPASS_ENABLE:
      macsec.physicalBypass = attr->value.booldata;
      break;
    default:
      res = SAI_STATUS_INVALID_PARAMETER;
      break;
  }
  return res;
}

sai_status_t get_macsec_attribute_fn(
    sai_object_id_t macsec_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  const auto& macsec = fs->macsecManager.get(macsec_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_MACSEC_ATTR_DIRECTION:
        attr[i].value.s32 = macsec.direction;
        break;
      case SAI_MACSEC_ATTR_PHYSICAL_BYPASS_ENABLE:
        attr[i].value.booldata = macsec.physicalBypass;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t create_macsec_port_fn(
    sai_object_id_t* macsec_port_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  sai_macsec_direction_t macsecDirection;
  sai_object_id_t linePortId;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_MACSEC_PORT_ATTR_PORT_ID:
        linePortId = attr_list[i].value.oid;
        break;
      case SAI_MACSEC_PORT_ATTR_MACSEC_DIRECTION:
        macsecDirection =
            static_cast<sai_macsec_direction_t>(attr_list[i].value.s32);
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }

  *macsec_port_id = fs->macsecPortManager.create(linePortId, macsecDirection);
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_macsec_port_fn(sai_object_id_t macsec_port_id) {
  FakeSai::getInstance()->macsecPortManager.remove(macsec_port_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_macsec_port_attribute_fn(
    sai_object_id_t macsec_port_id,
    const sai_attribute_t* /* attr */) {
  auto fs = FakeSai::getInstance();
  fs->macsecPortManager.get(macsec_port_id);
  // we don't currently support setting any attrs on the port.
  return SAI_STATUS_INVALID_PARAMETER;
}

sai_status_t get_macsec_port_attribute_fn(
    sai_object_id_t macsec_port_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  const auto& macsecPort = fs->macsecPortManager.get(macsec_port_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_MACSEC_PORT_ATTR_PORT_ID:
        attr[i].value.oid = macsecPort.portID;
        break;
      case SAI_MACSEC_PORT_ATTR_MACSEC_DIRECTION:
        attr[i].value.s32 = macsecPort.macsecDirection;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t create_macsec_sa_fn(
    sai_object_id_t* macsec_sa_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  sai_object_id_t scid;
  sai_uint8_t an;
  sai_macsec_auth_key_t authKey;
  sai_macsec_direction_t macsecDirection;
  sai_uint32_t ssci;
  sai_macsec_sak_t sak;
  sai_macsec_salt_t salt;
  std::optional<sai_uint64_t> minimumXpn;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_MACSEC_SA_ATTR_SC_ID:
        scid = attr_list[i].value.oid;
        break;
      case SAI_MACSEC_SA_ATTR_AN:
        an = attr_list[i].value.u8;
        break;
      case SAI_MACSEC_SA_ATTR_AUTH_KEY:
        std::copy(
            std::begin(attr_list[i].value.macsecauthkey),
            std::end(attr_list[i].value.macsecauthkey),
            authKey);
        break;
      case SAI_MACSEC_SA_ATTR_MACSEC_DIRECTION:
        macsecDirection =
            static_cast<sai_macsec_direction_t>(attr_list[i].value.s32);
        break;
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
      case SAI_MACSEC_SA_ATTR_MACSEC_SSCI:
        ssci = attr_list[i].value.s32;
        break;
#endif
      case SAI_MACSEC_SA_ATTR_SAK:
        std::copy(
            std::begin(attr_list[i].value.macsecsak),
            std::end(attr_list[i].value.macsecsak),
            sak);
        break;
      case SAI_MACSEC_SA_ATTR_SALT:
        std::copy(
            std::begin(attr_list[i].value.macsecsalt),
            std::end(attr_list[i].value.macsecsalt),
            salt);
        break;
      case SAI_MACSEC_SA_ATTR_MINIMUM_XPN:
        minimumXpn = attr_list[i].value.u64;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }

  *macsec_sa_id = fs->macsecSAManager.create(
      scid, an, authKey, macsecDirection, ssci, sak, salt);
  auto& macsecSA = fs->macsecSAManager.get(*macsec_sa_id);
  if (minimumXpn) {
    macsecSA.minimumXpn = minimumXpn.value();
  }

  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_macsec_sa_fn(sai_object_id_t macsec_sa_id) {
  FakeSai::getInstance()->macsecSAManager.remove(macsec_sa_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_macsec_sa_attribute_fn(
    sai_object_id_t macsec_sa_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& macsecSA = fs->macsecSAManager.get(macsec_sa_id);
  sai_status_t res = SAI_STATUS_SUCCESS;
  if (!attr) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  switch (attr->id) {
    case SAI_MACSEC_SA_ATTR_MINIMUM_XPN:
      macsecSA.minimumXpn = attr->value.u64;
      break;
    default:
      res = SAI_STATUS_INVALID_PARAMETER;
      break;
  }
  return res;
}

sai_status_t get_macsec_sa_attribute_fn(
    sai_object_id_t macsec_sa_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  const auto& macsecSA = fs->macsecSAManager.get(macsec_sa_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_MACSEC_SA_ATTR_MINIMUM_XPN:
        attr[i].value.u64 = macsecSA.minimumXpn;
        break;
#if SAI_API_VERSION >= SAI_VERSION(1, 8, 1)
      case SAI_MACSEC_SA_ATTR_CURRENT_XPN:
        attr[i].value.u64 = macsecSA.currentXpn;
        break;
#endif
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t create_macsec_sc_fn(
    sai_object_id_t* macsec_sc_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  sai_uint64_t sci;
  sai_macsec_direction_t macsecDirection;
  sai_object_id_t flowId;
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
  sai_macsec_cipher_suite_t cipherSuite;
#endif
  std::optional<bool> sciEnable;
  std::optional<bool> replayProtectionEnable;
  std::optional<sai_uint32_t> replayProtectionWindow;
  std::optional<sai_uint8_t> sectagOffset;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_MACSEC_SC_ATTR_MACSEC_SCI:
        sci = attr_list[i].value.s64;
        break;
      case SAI_MACSEC_SC_ATTR_MACSEC_DIRECTION:
        macsecDirection =
            static_cast<sai_macsec_direction_t>(attr_list[i].value.s32);
        break;
      case SAI_MACSEC_SC_ATTR_FLOW_ID:
        flowId = attr_list[i].value.s64;
        break;
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
      case SAI_MACSEC_SC_ATTR_MACSEC_CIPHER_SUITE:
        cipherSuite =
            static_cast<sai_macsec_cipher_suite_t>(attr_list[i].value.s32);
        break;
#endif
      case SAI_MACSEC_SC_ATTR_MACSEC_EXPLICIT_SCI_ENABLE:
        sciEnable = attr_list[i].value.booldata;
        break;
      case SAI_MACSEC_SC_ATTR_MACSEC_REPLAY_PROTECTION_ENABLE:
        replayProtectionEnable = attr_list[i].value.booldata;
        break;
      case SAI_MACSEC_SC_ATTR_MACSEC_REPLAY_PROTECTION_WINDOW:
        replayProtectionWindow = attr_list[i].value.u32;
        break;
      case SAI_MACSEC_SC_ATTR_MACSEC_SECTAG_OFFSET:
        sectagOffset = attr_list[i].value.u8;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  *macsec_sc_id = fs->macsecSCManager.create(
      sci,
      macsecDirection,
      flowId
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
      ,
      cipherSuite
#endif
  );
  auto& macsecSC = fs->macsecSCManager.get(*macsec_sc_id);

  if (sciEnable) {
    macsecSC.sciEnable = sciEnable.value();
  }
  if (replayProtectionEnable) {
    macsecSC.replayProtectionEnable = replayProtectionEnable.value();
  }
  if (replayProtectionWindow) {
    macsecSC.replayProtectionWindow = replayProtectionWindow.value();
  }
  if (sectagOffset) {
    macsecSC.sectagOffset = sectagOffset.value();
  }

  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_macsec_sc_fn(sai_object_id_t macsec_sc_id) {
  FakeSai::getInstance()->macsecSCManager.remove(macsec_sc_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_macsec_sc_attribute_fn(
    sai_object_id_t macsec_sc_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& macsecSC = fs->macsecSCManager.get(macsec_sc_id);
  sai_status_t res = SAI_STATUS_SUCCESS;
  if (!attr) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  switch (attr->id) {
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
    case SAI_MACSEC_SC_ATTR_MACSEC_CIPHER_SUITE:
      macsecSC.cipherSuite =
          static_cast<sai_macsec_cipher_suite_t>(attr->value.s32);
      break;
#endif
    case SAI_MACSEC_SC_ATTR_MACSEC_EXPLICIT_SCI_ENABLE:
      macsecSC.sciEnable = attr->value.booldata;
      break;
    case SAI_MACSEC_SC_ATTR_MACSEC_REPLAY_PROTECTION_ENABLE:
      macsecSC.replayProtectionEnable = attr->value.booldata;
      break;
    case SAI_MACSEC_SC_ATTR_MACSEC_REPLAY_PROTECTION_WINDOW:
      macsecSC.replayProtectionWindow = attr->value.s64;
      break;
    case SAI_MACSEC_SC_ATTR_MACSEC_SECTAG_OFFSET:
      macsecSC.sectagOffset = attr->value.u8;
      break;
    default:
      res = SAI_STATUS_INVALID_PARAMETER;
      break;
  }
  return res;
}

sai_status_t get_macsec_sc_attribute_fn(
    sai_object_id_t macsec_sc_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  const auto& macsecSC = fs->macsecSCManager.get(macsec_sc_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_MACSEC_SC_ATTR_MACSEC_SCI:
        attr[i].value.s64 = macsecSC.sci;
        break;
      case SAI_MACSEC_SC_ATTR_MACSEC_DIRECTION:
        attr[i].value.s32 = macsecSC.macsecDirection;
        break;
      case SAI_MACSEC_SC_ATTR_ACTIVE_EGRESS_SA_ID:
        attr[i].value.oid = macsecSC.activeEgressSAID;
        break;
      case SAI_MACSEC_SC_ATTR_FLOW_ID:
        attr[i].value.oid = macsecSC.flowId;
        break;
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
      case SAI_MACSEC_SC_ATTR_MACSEC_CIPHER_SUITE:
        attr[i].value.s32 = macsecSC.cipherSuite;
        break;
#endif
      case SAI_MACSEC_SC_ATTR_MACSEC_EXPLICIT_SCI_ENABLE:
        attr[i].value.booldata = macsecSC.sciEnable;
        break;
      case SAI_MACSEC_SC_ATTR_MACSEC_REPLAY_PROTECTION_ENABLE:
        attr[i].value.booldata = macsecSC.replayProtectionEnable;
        break;
      case SAI_MACSEC_SC_ATTR_MACSEC_REPLAY_PROTECTION_WINDOW:
        attr[i].value.s32 = macsecSC.replayProtectionWindow;
        break;
      case SAI_MACSEC_SC_ATTR_MACSEC_SECTAG_OFFSET:
        attr[i].value.u8 = macsecSC.sectagOffset;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t create_macsec_flow_fn(
    sai_object_id_t* macsec_flow_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  sai_macsec_direction_t direction;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_MACSEC_ATTR_DIRECTION:
        direction = static_cast<sai_macsec_direction_t>(attr_list[i].value.s32);
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }

  *macsec_flow_id = fs->macsecFlowManager.create(direction);
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_macsec_flow_fn(sai_object_id_t macsec_flow_id) {
  FakeSai::getInstance()->macsecFlowManager.remove(macsec_flow_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_macsec_flow_attribute_fn(
    sai_object_id_t macsec_flow_id,
    const sai_attribute_t* /* attr */) {
  auto fs = FakeSai::getInstance();
  fs->macsecFlowManager.get(macsec_flow_id);
  // don't currently use any settable attributes
  return SAI_STATUS_INVALID_PARAMETER;
}

sai_status_t get_macsec_flow_attribute_fn(
    sai_object_id_t macsec_flow_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  const auto& macsecFlow = fs->macsecFlowManager.get(macsec_flow_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_MACSEC_FLOW_ATTR_MACSEC_DIRECTION:
        attr[i].value.s32 = macsecFlow.macsecDirection;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

static sai_macsec_api_t _macsec_api;

void populate_macsec_api(sai_macsec_api_t** macsec_api) {
  _macsec_api.create_macsec = &create_macsec_fn;
  _macsec_api.remove_macsec = &remove_macsec_fn;
  _macsec_api.set_macsec_attribute = &set_macsec_attribute_fn;
  _macsec_api.get_macsec_attribute = &get_macsec_attribute_fn;

  _macsec_api.create_macsec_port = &create_macsec_port_fn;
  _macsec_api.remove_macsec_port = &remove_macsec_port_fn;
  _macsec_api.set_macsec_port_attribute = &set_macsec_port_attribute_fn;
  _macsec_api.get_macsec_port_attribute = &get_macsec_port_attribute_fn;

  _macsec_api.create_macsec_sa = &create_macsec_sa_fn;
  _macsec_api.remove_macsec_sa = &remove_macsec_sa_fn;
  _macsec_api.set_macsec_sa_attribute = &set_macsec_sa_attribute_fn;
  _macsec_api.get_macsec_sa_attribute = &get_macsec_sa_attribute_fn;

  _macsec_api.create_macsec_sc = &create_macsec_sc_fn;
  _macsec_api.remove_macsec_sc = &remove_macsec_sc_fn;
  _macsec_api.set_macsec_sc_attribute = &set_macsec_sc_attribute_fn;
  _macsec_api.get_macsec_sc_attribute = &get_macsec_sc_attribute_fn;

  _macsec_api.create_macsec_flow = &create_macsec_flow_fn;
  _macsec_api.remove_macsec_flow = &remove_macsec_flow_fn;
  _macsec_api.set_macsec_flow_attribute = &set_macsec_flow_attribute_fn;
  _macsec_api.get_macsec_flow_attribute = &get_macsec_flow_attribute_fn;

  // TODO(ccpowers): add stats apis (get/ext/clear)

  *macsec_api = &_macsec_api;
}

} // namespace facebook::fboss
