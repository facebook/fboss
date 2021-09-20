// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/usb/tests/MockTransceiverI2CApi.h"
#include <exception>
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

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
    uint8_t i2cAddress,
    int offset,
    int len,
    uint8_t* buf) {
  EXPECT_TRUE(i2cAddress == 0x50 || i2cAddress == 0x51);
  // Throw a forced exception when asked to do so
  if (throwReadExceptionForMgmtInterface_ ||
      (len != 1 && throwReadExceptionForDomQuery_)) {
    throw std::exception();
  }
  if (offset == 0) {
    if (overridenMgmtInterface_.find(module) != overridenMgmtInterface_.end()) {
      buf[0] = overridenMgmtInterface_[module];
    } else {
      buf[0] = uint8_t(TransceiverModuleIdentifier::QSFP28);
    }
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
}
} // namespace facebook::fboss
