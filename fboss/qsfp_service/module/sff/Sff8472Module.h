// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/qsfp_service/module/QsfpModule.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook {
namespace fboss {

enum class Sff8472Field;

class Sff8472Module : public QsfpModule {
 public:
  explicit Sff8472Module(
      TransceiverManager* transceiverManager,
      std::unique_ptr<TransceiverImpl> qsfpImpl,
      unsigned int portsPerTransceiver);
  virtual ~Sff8472Module() override;

  /*
   * Return a valid type.
   */
  TransceiverType type() const override {
    return TransceiverType::SFP;
  }

  /*
   * Return the spec this transceiver follows.
   */
  TransceiverManagementInterface managementInterface() const override {
    return TransceiverManagementInterface::SFF8472;
  }

  RawDOMData getRawDOMData() override {
    return RawDOMData();
  }

  DOMDataUnion getDOMDataUnion() override {
    return DOMDataUnion();
  }

  unsigned int numHostLanes() const override {
    return 1;
  }

  unsigned int numMediaLanes() const override {
    return 1;
  }

  void customizeTransceiverLocked(cfg::PortSpeed) override {}

  const uint8_t* getQsfpValuePtr(int, int, int) const override {
    return nullptr;
  }

  void setQsfpFlatMem() override {}

  void setPowerOverrideIfSupported(PowerControlState) override {}

  TransmitterTechnology getQsfpTransmitterTechnology() const override {
    return TransmitterTechnology::OPTICAL;
  }

  std::optional<AlarmThreshold> getThresholdInfo() override {
    return std::nullopt;
  }

  /*
   * returns individual sensor values after scaling
   */
  double getSfpSensor(Sff8472Field field, double (*conversion)(uint16_t value));

  GlobalSensors getSensorInfo() override;

  bool getSensorsPerChanInfo(std::vector<Channel>& channels) override;

  bool getSignalsPerMediaLane(std::vector<MediaLaneSignals>&) override {
    return false;
  }

  bool getSignalsPerHostLane(std::vector<HostLaneSignals>&) override {
    return false;
  }

  Vendor getVendorInfo() override {
    return Vendor();
  }

  Cable getCableInfo() override {
    return Cable();
  }

  TransceiverSettings getTransceiverSettingsInfo() override;

  PowerControlState getPowerControlValue() override {
    return PowerControlState::HIGH_POWER_OVERRIDE;
  }

  virtual SignalFlags getSignalFlagInfo() override {
    return SignalFlags();
  }

  ExtendedSpecComplianceCode getExtendedSpecificationComplianceCode()
      const override {
    return ExtendedSpecComplianceCode::UNKNOWN;
  }

  TransceiverModuleIdentifier getIdentifier() override {
    return TransceiverModuleIdentifier::SFP_PLUS;
  }

  ModuleStatus getModuleStatus() override {
    return ModuleStatus();
  }

  void updateQsfpData(bool allPages = true) override;

  double getQsfpDACLength() const override {
    return 0;
  }

 protected:
  uint8_t a0LowerPage_[MAX_QSFP_PAGE_SIZE] = {0};
  uint8_t a2LowerPage_[MAX_QSFP_PAGE_SIZE] = {0};

  bool getMediaInterfaceId(std::vector<MediaInterfaceId>& mediaInterface);
  const uint8_t* getSfpValuePtr(
      int dataAddress,
      int offset,
      int length,
      uint8_t transceiverI2CAddress) const;
  void getSfpValue(
      int dataAddress,
      int offset,
      int length,
      uint8_t transceiverI2CAddress,
      uint8_t* data) const;
  uint8_t getSettingsValue(Sff8472Field field, uint8_t mask = 0xff) const;

  Ethernet10GComplianceCode getEthernet10GComplianceCode() const;
};

} // namespace fboss
} // namespace facebook
