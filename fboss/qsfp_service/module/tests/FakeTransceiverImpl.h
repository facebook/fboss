// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/qsfp_service/module/TransceiverImpl.h"

namespace facebook {
namespace fboss {

class TransceiverManager;

// This class contains a fake implementation of a transceiver. It overrides the
// readTransceiver, writeTransceiver and some other methods. It uses a fake
// eeprom map, the reads read from the map and the writes modify the map.
class FakeTransceiverImpl : public TransceiverImpl {
 public:
  FakeTransceiverImpl(
      int module,
      std::map<uint8_t, std::array<uint8_t, 128>>& lowerPage,
      std::map<uint8_t, std::map<int, std::array<uint8_t, 128>>>& upperPages,
      TransceiverManager* mgr) {
    module_ = module;
    moduleName_ = folly::to<std::string>(module);
    lowerPages_.insert(lowerPage.begin(), lowerPage.end());
    upperPages_.insert(upperPages.begin(), upperPages.end());
    tcvrManager_ = mgr;
  }
  /* This function is used to read the SFP EEprom */
  int readTransceiver(
      const TransceiverAccessParameter& param,
      uint8_t* fieldValue,
      const int field) override;
  /* write to the eeprom (usually to change the page setting) */
  int writeTransceiver(
      const TransceiverAccessParameter& param,
      const uint8_t* fieldValue,
      uint64_t sleep,
      const int field) override;
  /* This function detects if a SFP is present on the particular port */
  bool detectTransceiver() override;
  /* Returns the name for the port */
  folly::StringPiece getName() override;
  int getNum() const override;
  void triggerQsfpHardReset() override;
  void updateTransceiverState(TransceiverStateMachineEvent event) override;

 protected:
  // Provide distinct EEPROM contents for a specific (bank, page) so multi-bank
  // (CPO) reads can be exercised. When the bank-select register (byte 126) is
  // set to a bank registered here, reads of that page return this data;
  // otherwise reads fall back to the bank-agnostic upperPages_ contents.
  void setBankedPage(uint8_t bank, int page, std::array<uint8_t, 128> data) {
    bankedPages_[bank][page] = data;
  }

 private:
  int module_{0};
  std::string moduleName_;
  int page_{0};
  uint8_t selectedBank_{0};
  std::map<uint8_t, std::map<int, std::array<uint8_t, 128>>> upperPages_;
  // Optional per-bank page overrides, indexed [bank][page]. Empty by default,
  // so single-bank fixtures behave exactly as before.
  std::map<uint8_t, std::map<int, std::array<uint8_t, 128>>> bankedPages_;
  std::map<uint8_t, std::array<uint8_t, 128>> lowerPages_;
  TransceiverManager* tcvrManager_;
};

class SffDacTransceiver : public FakeTransceiverImpl {
 public:
  explicit SffDacTransceiver(int module, TransceiverManager* mgr);
};

class SffCwdm4Transceiver : public FakeTransceiverImpl {
 public:
  explicit SffCwdm4Transceiver(int module, TransceiverManager* mgr);
};

class SffFr1Transceiver : public FakeTransceiverImpl {
 public:
  explicit SffFr1Transceiver(int module, TransceiverManager* mgr);
};

class Sff200GCr4Transceiver : public FakeTransceiverImpl {
 public:
  explicit Sff200GCr4Transceiver(int module, TransceiverManager* mgr);
};

class BadSffCwdm4Transceiver : public FakeTransceiverImpl {
 public:
  explicit BadSffCwdm4Transceiver(int module, TransceiverManager* mgr);
};

class BadEepromSffCwdm4Transceiver : public FakeTransceiverImpl {
 public:
  explicit BadEepromSffCwdm4Transceiver(int module, TransceiverManager* mgr);
};

class MiniphotonOBOTransceiver : public FakeTransceiverImpl {
 public:
  explicit MiniphotonOBOTransceiver(int module, TransceiverManager* mgr);
};

class UnknownModuleIdentifierTransceiver : public FakeTransceiverImpl {
 public:
  explicit UnknownModuleIdentifierTransceiver(
      int module,
      TransceiverManager* mgr);
};

class Cmis200GTransceiver : public FakeTransceiverImpl {
 public:
  explicit Cmis200GTransceiver(int module, TransceiverManager* mgr);
};

class BadCmis200GTransceiver : public FakeTransceiverImpl {
 public:
  explicit BadCmis200GTransceiver(int module, TransceiverManager* mgr);
};

class Cmis400GLr4Transceiver : public FakeTransceiverImpl {
 public:
  explicit Cmis400GLr4Transceiver(int module, TransceiverManager* mgr);
};

class CmisFlatMemTransceiver : public FakeTransceiverImpl {
 public:
  explicit CmisFlatMemTransceiver(int module, TransceiverManager* mgr);
};

class Sfp10GTransceiver : public FakeTransceiverImpl {
 public:
  explicit Sfp10GTransceiver(int module, TransceiverManager* mgr);
};

class Sfp10GBaseTTransceiver : public FakeTransceiverImpl {
 public:
  explicit Sfp10GBaseTTransceiver(int module, TransceiverManager* mgr);
};

class Cmis400GCr8Transceiver : public FakeTransceiverImpl {
 public:
  explicit Cmis400GCr8Transceiver(int module, TransceiverManager* mgr);
};

class Cmis400GFr4MultiPortTransceiver : public FakeTransceiverImpl {
 public:
  explicit Cmis400GFr4MultiPortTransceiver(int module, TransceiverManager* mgr);
};

class Cmis2x400GFr4Transceiver : public FakeTransceiverImpl {
 public:
  explicit Cmis2x400GFr4Transceiver(int module, TransceiverManager* mgr);
};

class Cmis2x400GFr4LiteTransceiver : public FakeTransceiverImpl {
 public:
  explicit Cmis2x400GFr4LiteTransceiver(int module, TransceiverManager* mgr);
};

class Cmis2x400GFr4LpoTransceiver : public FakeTransceiverImpl {
 public:
  explicit Cmis2x400GFr4LpoTransceiver(int module, TransceiverManager* mgr);
};

class Cmis800GZrTransceiver : public FakeTransceiverImpl {
 public:
  explicit Cmis800GZrTransceiver(int module, TransceiverManager* mgr);
};

class Cmis2x400GDr4Transceiver : public FakeTransceiverImpl {
 public:
  explicit Cmis2x400GDr4Transceiver(int module, TransceiverManager* mgr);
};

// Custom transceiver for testing CWDM4_100G temperature thresholds
class SffCwdm4TempTransceiver : public FakeTransceiverImpl {
 public:
  explicit SffCwdm4TempTransceiver(int module, TransceiverManager* mgr);

  // Set the temperature value in the lower page
  void setTemperature(double tempValue);
};

class Cmis2x400GFr4WithMpiAlarmsTransceiver : public Cmis2x400GFr4Transceiver {
 public:
  explicit Cmis2x400GFr4WithMpiAlarmsTransceiver(
      int module,
      TransceiverManager* mgr);
};

class Cmis400GDr4Transceiver : public FakeTransceiverImpl {
 public:
  explicit Cmis400GDr4Transceiver(int module, TransceiverManager* mgr);
};

class Cmis2x800GDr4Transceiver : public FakeTransceiverImpl {
 public:
  explicit Cmis2x800GDr4Transceiver(int module, TransceiverManager* mgr);
};

class CmisCpo6P4TDrTransceiver : public FakeTransceiverImpl {
 public:
  explicit CmisCpo6P4TDrTransceiver(int module, TransceiverManager* mgr);
};

// CPO module in READY state with distinct per-bank page 14h, for exercising the
// per-bank SNR (14h) read path.
class CmisCpo6P4TDrReadyTransceiver : public CmisCpo6P4TDrTransceiver {
 public:
  explicit CmisCpo6P4TDrReadyTransceiver(int module, TransceiverManager* mgr);
};

class CmisCredo800AEC : public FakeTransceiverImpl {
 public:
  explicit CmisCredo800AEC(int module, TransceiverManager* mgr);
};

class InvalidDatapathLaneStateTransceiver : public FakeTransceiverImpl {
 public:
  explicit InvalidDatapathLaneStateTransceiver(
      int module,
      TransceiverManager* mgr);
};

} // namespace fboss
} // namespace facebook
