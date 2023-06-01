// Copyright 2004-present Facebook. All Rights Reserved.

#include <folly/logging/xlog.h>
#include <exception>
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include "fboss/lib/usb/tests/MockTransceiverI2CApi.h"

namespace facebook::fboss {

bool MockTransceiverI2CApi::isPresent(unsigned int id) {
  // If the user has overridden the presence for this id, return that otherwise
  // return true by default
  if (overridenPresence_.find(id) != overridenPresence_.end()) {
    return overridenPresence_[id];
  }
  return true;
}

void MockTransceiverI2CApi::moduleRead(
    unsigned int module,
    const TransceiverAccessParameter& param,
    uint8_t* buf) {
  // I2C access should always have a i2cAddress
  CHECK(param.i2cAddress.has_value());
  auto i2cAddress = *(param.i2cAddress);
  auto offset = param.offset;
  auto len = param.len;
  EXPECT_TRUE(i2cAddress == 0x50 || i2cAddress == 0x51);
  // Throw a forced exception when asked to do so
  if (throwReadExceptionForMgmtInterface_ ||
      (len != 1 && throwReadExceptionForDomQuery_)) {
    throw std::exception();
  }
  if (offset == 0 && len == 1) {
    if (overridenMgmtInterface_.find(module) != overridenMgmtInterface_.end()) {
      buf[0] = overridenMgmtInterface_[module];
    } else {
      buf[0] = uint8_t(TransceiverModuleIdentifier::QSFP28);
    }
  } else if (const auto tcvrItor = overridenTransceivers_.find(module);
             tcvrItor != overridenTransceivers_.end()) {
    tcvrItor->second->readTransceiver(param, buf);
  } else {
    XLOG(ERR)
        << "moduleRead(module=" << module << ", i2cAddress=" << i2cAddress
        << ", offset=" << offset << ", len=" << len
        << "): FIXME: Reading undefined data. Returning zerod data as workaround.";
    memset(buf, 0, len);
  }
}

void MockTransceiverI2CApi::scanPresence(
    std::map<int32_t, ModulePresence>& presences) {
  for (auto& presence : presences) {
    if (isPresent(presence.first)) {
      presence.second = ModulePresence::PRESENT;
    } else {
      presence.second = ModulePresence::ABSENT;
    }
  }
}

void MockTransceiverI2CApi::overridePresence(unsigned int id, bool presence) {
  overridenPresence_[id] = presence;
}

void MockTransceiverI2CApi::overrideMgmtInterface(
    unsigned int id,
    uint8_t mgmt) {
  overridenMgmtInterface_[id] = mgmt;
  switch (mgmt) {
    case uint8_t(TransceiverModuleIdentifier::UNKNOWN):
      overridenTransceivers_[id] =
          std::make_unique<UnknownModuleIdentifierTransceiver>(id);
      break;
    case uint8_t(TransceiverModuleIdentifier::SFP_PLUS):
      overridenTransceivers_[id] = std::make_unique<Sfp10GTransceiver>(id);
      break;
    case uint8_t(TransceiverModuleIdentifier::QSFP):
    case uint8_t(TransceiverModuleIdentifier::QSFP_PLUS):
    case uint8_t(TransceiverModuleIdentifier::QSFP28):
      overridenTransceivers_[id] = std::make_unique<SffDacTransceiver>(id);
      break;
    case uint8_t(TransceiverModuleIdentifier::QSFP_DD):
    case uint8_t(TransceiverModuleIdentifier::QSFP_PLUS_CMIS):
      overridenTransceivers_[id] = std::make_unique<Cmis200GTransceiver>(id);
      break;
    case uint8_t(TransceiverModuleIdentifier::MINIPHOTON_OBO):
      overridenTransceivers_[id] =
          std::make_unique<MiniphotonOBOTransceiver>(id);
      break;
    case uint8_t(TransceiverModuleIdentifier::OSFP):
      overridenTransceivers_[id] =
          std::make_unique<Cmis2x400GFr4Transceiver>(id);
      break;
    default:
      overridenTransceivers_.erase(id);
      XLOG(ERR) << "Unknown transceiver module identifier " << mgmt
                << "for management interface " << id;
      break;
  }
}
} // namespace facebook::fboss
