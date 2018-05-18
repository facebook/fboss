#pragma once

#include <vector>

#include "fboss/agent/types.h"
#include "fboss/qsfp_service/sff/Transceiver.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook { namespace fboss {
class TransceiverManager {
 public:
  TransceiverManager() {};
  virtual ~TransceiverManager() {};
  virtual void initTransceiverMap() = 0;
  virtual void getTransceiversInfo(std::map<int32_t, TransceiverInfo>& info,
    std::unique_ptr<std::vector<int32_t>> ids) = 0;
  virtual void getTransceiversRawDOMData(std::map<int32_t, RawDOMData>& info,
    std::unique_ptr<std::vector<int32_t>> ids) = 0;
  virtual void customizeTransceiver(int32_t idx, cfg::PortSpeed speed) = 0;
  virtual void syncPorts(
    std::map<int32_t, TransceiverInfo>& info,
    std::unique_ptr<std::map<int32_t, PortStatus>> ports) = 0;

  bool isValidTransceiver(int32_t id) {
    return id < transceivers_.size() && id >= 0;
  }
  virtual int getNumQsfpModules() = 0;
  virtual void refreshTransceivers() = 0;
  virtual int numPortsPerTransceiver() = 0;
 private:
  // Forbidden copy constructor and assignment operator
  TransceiverManager(TransceiverManager const &) = delete;
  TransceiverManager& operator=(TransceiverManager const &) = delete;
 protected:
  std::vector<std::unique_ptr<Transceiver>> transceivers_;
};
}} // facebook::fboss
