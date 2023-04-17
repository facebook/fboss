// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/qsfp_service/module/TransceiverImpl.h"

namespace facebook {
namespace fboss {

// This class contains a fake implementation of a transceiver. It overrides the
// readTransceiver, writeTransceiver and some other methods. It uses a fake
// eeprom map, the reads read from the map and the writes modify the map.
class FakeTransceiverImpl : public TransceiverImpl {
 public:
  FakeTransceiverImpl(
      int module,
      std::map<uint8_t, std::array<uint8_t, 128>>& lowerPage,
      std::map<uint8_t, std::map<int, std::array<uint8_t, 128>>>& upperPages) {
    module_ = module;
    moduleName_ = folly::to<std::string>(module);
    lowerPages_.insert(lowerPage.begin(), lowerPage.end());
    upperPages_.insert(upperPages.begin(), upperPages.end());
  }
  /* This function is used to read the SFP EEprom */
  int readTransceiver(
      const TransceiverAccessParameter& param,
      uint8_t* fieldValue) override;
  /* write to the eeprom (usually to change the page setting) */
  int writeTransceiver(
      const TransceiverAccessParameter& param,
      uint8_t* fieldValue) override;
  /* This function detects if a SFP is present on the particular port */
  bool detectTransceiver() override;
  /* Returns the name for the port */
  folly::StringPiece getName() override;
  int getNum() const override;

 private:
  int module_{0};
  std::string moduleName_;
  int page_{0};
  std::map<uint8_t, std::map<int, std::array<uint8_t, 128>>> upperPages_;
  std::map<uint8_t, std::array<uint8_t, 128>> lowerPages_;
};

class SffDacTransceiver : public FakeTransceiverImpl {
 public:
  explicit SffDacTransceiver(int module);
};

class SffCwdm4Transceiver : public FakeTransceiverImpl {
 public:
  explicit SffCwdm4Transceiver(int module);
};

class SffFr1Transceiver : public FakeTransceiverImpl {
 public:
  explicit SffFr1Transceiver(int module);
};

class Sff200GCr4Transceiver : public FakeTransceiverImpl {
 public:
  explicit Sff200GCr4Transceiver(int module);
};

class BadSffCwdm4Transceiver : public FakeTransceiverImpl {
 public:
  explicit BadSffCwdm4Transceiver(int module);
};

class BadEepromSffCwdm4Transceiver : public FakeTransceiverImpl {
 public:
  explicit BadEepromSffCwdm4Transceiver(int module);
};

class MiniphotonOBOTransceiver : public FakeTransceiverImpl {
 public:
  explicit MiniphotonOBOTransceiver(int module);
};

class UnknownModuleIdentifierTransceiver : public FakeTransceiverImpl {
 public:
  explicit UnknownModuleIdentifierTransceiver(int module);
};

class Cmis200GTransceiver : public FakeTransceiverImpl {
 public:
  explicit Cmis200GTransceiver(int module);
};

class BadCmis200GTransceiver : public FakeTransceiverImpl {
 public:
  explicit BadCmis200GTransceiver(int module);
};

class Cmis400GLr4Transceiver : public FakeTransceiverImpl {
 public:
  explicit Cmis400GLr4Transceiver(int module);
};

class CmisFlatMemTransceiver : public FakeTransceiverImpl {
 public:
  explicit CmisFlatMemTransceiver(int module);
};

class Sfp10GTransceiver : public FakeTransceiverImpl {
 public:
  explicit Sfp10GTransceiver(int module);
};

class Cmis400GCr8Transceiver : public FakeTransceiverImpl {
 public:
  explicit Cmis400GCr8Transceiver(int module);
};

class Cmis400GFr4MultiPortTransceiver : public FakeTransceiverImpl {
 public:
  explicit Cmis400GFr4MultiPortTransceiver(int module);
};

} // namespace fboss
} // namespace facebook
