#pragma once

#include <boost/container/flat_map.hpp>

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/lib/i2c/gen-cpp2/i2c_controller_stats_types.h"
#include "fboss/lib/usb/WedgeI2CBus.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeI2CBusLock.h"
#include "fboss/qsfp_service/TransceiverManager.h"

namespace facebook { namespace fboss {
class WedgeManager : public TransceiverManager {
 public:
  using TransceiverMap = std::map<int32_t, TransceiverInfo>;
  using PortMap = std::map<int32_t, PortStatus>;
  using PortNameMap = std::map<std::string, int32_t>;
  using PortGroups = std::map<int32_t, std::set<cfg::Port>>;

  explicit WedgeManager(
      std::unique_ptr<TransceiverPlatformApi> api,
      std::unique_ptr<PlatformMapping> platformMapping);
  ~WedgeManager() override {}

  void initTransceiverMap() override;
  void getTransceiversInfo(TransceiverMap& info,
    std::unique_ptr<std::vector<int32_t>> ids) override;
  void getTransceiversRawDOMData(
    std::map<int32_t, RawDOMData>& info,
    std::unique_ptr<std::vector<int32_t>> ids) override;
  void getTransceiversDOMDataUnion(
    std::map<int32_t, DOMDataUnion>& info,
    std::unique_ptr<std::vector<int32_t>> ids) override;
  void customizeTransceiver(int32_t idx, cfg::PortSpeed speed) override;
  void syncPorts(TransceiverMap& info, std::unique_ptr<PortMap> ports) override;

  int getNumQsfpModules() override {
    return 16;
  }
  int numPortsPerTransceiver() override {
    // most of our platforms specify a port per channel in a qsfp
    return 4;
  }
  void refreshTransceivers() override;

  int scanTransceiverPresence(
      std::unique_ptr<std::vector<int32_t>> ids) override;

  /* The function gets the i2c gets the i2c transaction stats. This class
   * will be inherited by platform specific class like Minipack16QManager from
   * where this function will be called. This function uses platform
   * specific I2c class routing to get these counters
   */
  std::vector<std::reference_wrapper<const I2cControllerStats>>
  getI2cControllerStats() const override {
    return wedgeI2cBus_->getI2cControllerStats();
  }

  /* Get the i2c transaction counters from TranscieverManager base class
   * and update to fbagent. The TransceieverManager base class is inherited
   * by platform speficic Transaceiver Manager class like WedgeQManager.
   * That class has the function to get the I2c transaction status
   */
  void publishI2cTransactionStats() override;

  // This function will bring all the transceivers out of reset, making use
  // of the specific implementation from each platform. Platforms that bring
  // transceiver out of reset by default will stay no op.
  void clearAllTransceiverReset();

 protected:
  virtual std::unique_ptr<TransceiverI2CApi> getI2CBus();
  void updateTransceiverMap();
  std::unique_ptr<TransceiverI2CApi>
      wedgeI2cBus_; /* thread safe handle to access bus */

  std::unique_ptr<AgentConfig> config_;
  const std::unique_ptr<const PlatformMapping> platformMapping_;
  PortNameMap portName2Module_;
  PortGroups portGroupMap_;

  folly::Synchronized<std::map<TransceiverID, std::map<uint32_t, PortStatus>>>
      ports_;

 private:
  void loadConfig() override;
  // Forbidden copy constructor and assignment operator
  WedgeManager(WedgeManager const &) = delete;
  WedgeManager& operator=(WedgeManager const &) = delete;
};
}} // facebook::fboss
