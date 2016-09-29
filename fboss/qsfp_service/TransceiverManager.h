#pragma once

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
};
}} // facebook::fboss
