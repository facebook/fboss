#pragma once

#include <boost/container/flat_map.hpp>

#include "fboss/agent/AgentConfig.h"
#include "fboss/lib/usb/WedgeI2CBus.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeI2CBusLock.h"
#include "fboss/qsfp_service/TransceiverManager.h"
#include "fboss/lib/usb/TransceiverPlatformApi.h"

namespace facebook { namespace fboss {
class WedgeManager : public TransceiverManager {
 public:
  using TransceiverMap = std::map<int32_t, TransceiverInfo>;
  using PortMap = std::map<int32_t, PortStatus>;
  using PortNameMap = std::map<std::string, int32_t>;
  using PortGroups = std::map<int32_t, std::set<cfg::Port>>;

  WedgeManager(std::unique_ptr<TransceiverPlatformApi> api = nullptr);
  ~WedgeManager() override {}

  void loadConfig() override {}
  void initTransceiverMap() override;
  void getTransceiversInfo(TransceiverMap& info,
    std::unique_ptr<std::vector<int32_t>> ids) override;
  void getTransceiversRawDOMData(
    std::map<int32_t, RawDOMData>& info,
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

 protected:
  virtual std::unique_ptr<TransceiverI2CApi> getI2CBus();
  std::unique_ptr<TransceiverI2CApi>
      wedgeI2cBus_; /* thread safe handle to access bus */

  /* This variable stores the TransceiverPlatformApi object for controlling
   * the QSFP devies on board. This handle is populated from this class
   * constructor
   */
  std::unique_ptr<TransceiverPlatformApi>  qsfpPlatApi_;
  std::unique_ptr<AgentConfig> config_;
  PortNameMap portName2Module_;
  PortGroups portGroupMap_;

 private:
  // Forbidden copy constructor and assignment operator
  WedgeManager(WedgeManager const &) = delete;
  WedgeManager& operator=(WedgeManager const &) = delete;
};
}} // facebook::fboss
