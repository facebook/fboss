// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/qsfp_service/module/QsfpModule.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook {
namespace fboss {

enum class Sff8472Field;

enum class Sff8472Pages : int {
  LOWER = -1,
};

class Sff8472Module : public QsfpModule {
 public:
  explicit Sff8472Module(
      TransceiverManager* transceiverManager,
      std::unique_ptr<TransceiverImpl> qsfpImpl);
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

  virtual bool ensureTransceiverReadyLocked() override {
    return true;
  }

  const uint8_t* getQsfpValuePtr(int, int, int) const override {
    return nullptr;
  }

  void setQsfpFlatMem() override {}

  void setPowerOverrideIfSupportedLocked(PowerControlState) override {}

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

  bool getSignalsPerMediaLane(std::vector<MediaLaneSignals>&) override;

  bool getSignalsPerHostLane(std::vector<HostLaneSignals>&) override {
    return false;
  }

  /*
   * Gather media side per lane configuration settings and return false when it
   * fails
   */
  bool getMediaLaneSettings(std::vector<MediaLaneSettings>& laneSettings);

  std::string getSfpString(Sff8472Field flag) const;

  Vendor getVendorInfo() override;

  Cable getCableInfo() override {
    return Cable();
  }

  TransceiverSettings getTransceiverSettingsInfo() override;

  PowerControlState getPowerControlValue() override {
    return PowerControlState::HIGH_POWER_OVERRIDE;
  }

  virtual SignalFlags getSignalFlagInfo() override;

  TransceiverModuleIdentifier getIdentifier() override {
    return TransceiverModuleIdentifier::SFP_PLUS;
  }

  ModuleStatus getModuleStatus() override {
    return ModuleStatus();
  }

  void updateQsfpData(bool allPages = true) override;

  bool remediateFlakyTransceiver() override;

  double getQsfpDACLength() const override {
    return 0;
  }

  bool supportRemediate() override {
    return true;
  }

  void resetDataPath() override {
    // no-op
  }

  /* Helper function to read/write a SFF8472Field. The function will extract the
   * i2cAddress, page number, offset and length information from the
   * Sff8472Field and then make the corresponding qsfpImpl->readTransceiver and
   * qsfpImpl->writeTransceiver calls. The user should avoid making direct calls
   * to qsfpImpl->read/writeTransceiver and instead do register IO using
   * readSff8472Field/writeSff8472Field helper functions. */
  void readSff8472Field(Sff8472Field field, uint8_t* data);
  void writeSff8472Field(Sff8472Field field, uint8_t* data);

 protected:
  uint8_t a0LowerPage_[MAX_QSFP_PAGE_SIZE] = {0};
  uint8_t a2LowerPage_[MAX_QSFP_PAGE_SIZE] = {0};

  bool getMediaInterfaceId(
      std::vector<MediaInterfaceId>& mediaInterface) override;
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
