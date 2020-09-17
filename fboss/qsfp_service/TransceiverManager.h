#pragma once

#include <vector>
#include <map>

#include <folly/Synchronized.h>

#include "fboss/agent/types.h"
#include "fboss/qsfp_service/module/Transceiver.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/lib/i2c/gen-cpp2/i2c_controller_stats_types.h"
#include "fboss/lib/usb/TransceiverPlatformApi.h"

namespace facebook { namespace fboss {
class TransceiverManager {
 public:
  TransceiverManager(std::unique_ptr<TransceiverPlatformApi> api) :
    qsfpPlatApi_(std::move(api)) {};
  virtual ~TransceiverManager() {};
  virtual void initTransceiverMap() = 0;
  virtual void getTransceiversInfo(std::map<int32_t, TransceiverInfo>& info,
    std::unique_ptr<std::vector<int32_t>> ids) = 0;
  virtual void getTransceiversRawDOMData(std::map<int32_t, RawDOMData>& info,
    std::unique_ptr<std::vector<int32_t>> ids) = 0;
  virtual void getTransceiversDOMDataUnion(std::map<int32_t, DOMDataUnion>& info,
    std::unique_ptr<std::vector<int32_t>> ids) = 0;
  virtual void customizeTransceiver(int32_t idx, cfg::PortSpeed speed) = 0;
  virtual void syncPorts(
    std::map<int32_t, TransceiverInfo>& info,
    std::unique_ptr<std::map<int32_t, PortStatus>> ports) = 0;

  bool isValidTransceiver(int32_t id) {
    return id < getNumQsfpModules() && id >= 0;
  }
  virtual int getNumQsfpModules() = 0;
  virtual void refreshTransceivers() = 0;
  virtual int scanTransceiverPresence(
      std::unique_ptr<std::vector<int32_t>> ids) = 0;
  virtual int numPortsPerTransceiver() = 0;

  /* Virtual function to return the i2c transactions stats in a platform.
   * This will be overridden by derived classes which are platform specific
   * and has the platform specific implementation for this counter
   */
  virtual std::vector<std::reference_wrapper<const I2cControllerStats>>
    getI2cControllerStats() const = 0;

  /* Virtual function to update the I2c transaction stats to the ServiceData
   * object from where it will get picked up by FbAgent.
   * Implementation - The TransceieverManager base class is inherited
   * by platform speficic Transaceiver Manager class like WedgeManager.
   * That class has the function to get the I2c transaction status
   */
  virtual void publishI2cTransactionStats() = 0;

  TransceiverPlatformApi* getQsfpPlatformApi() {
    return qsfpPlatApi_.get();
  }

 private:
  // Forbidden copy constructor and assignment operator
  TransceiverManager(TransceiverManager const &) = delete;
  TransceiverManager& operator=(TransceiverManager const &) = delete;
 protected:
  virtual void loadConfig() = 0;
  folly::Synchronized<std::map<TransceiverID, std::unique_ptr<Transceiver>>> transceivers_;
  /* This variable stores the TransceiverPlatformApi object for controlling
   * the QSFP devies on board. This handle is populated from this class
   * constructor
   */
  std::unique_ptr<TransceiverPlatformApi>  qsfpPlatApi_;
};
}} // facebook::fboss
