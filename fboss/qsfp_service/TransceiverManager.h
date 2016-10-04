#pragma once

#include <boost/container/flat_map.hpp>

#include "fboss/agent/types.h"
#include "fboss/qsfp_service/sff/Transceiver.h"

namespace facebook { namespace fboss {
class TransceiverManager {
 public:
  TransceiverManager() {};
  virtual ~TransceiverManager() {};
  virtual void initTransceiverMap() = 0;
 private:
  // Forbidden copy constructor and assignment operator
  TransceiverManager(TransceiverManager const &) = delete;
  TransceiverManager& operator=(TransceiverManager const &) = delete;
 protected:
  boost::container::flat_map<TransceiverID,
   std::unique_ptr<Transceiver>> transceivers_;
};
}} // facebook::fboss
