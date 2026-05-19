// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/module/Transceiver.h"

#include <chrono>
#include <map>
#include <memory>
#include <mutex>

namespace facebook {
namespace fboss {

class TransceiverManager;

/**
 * CpoDomMonitor manages Digital Optical Monitoring (DOM) data collection
 * for Co-Packaged Optics (CPO) platforms. It separates OE (Optical Engine)
 * and ELSFP (External Laser SFP) DOM readings.
 */
class CpoDomMonitor {
 public:
  explicit CpoDomMonitor(TransceiverManager* tcvrMgr);
  ~CpoDomMonitor() = default;

  /**
   * Refresh CPO DOM data for all CPO-enabled transceivers.
   * Called periodically by TransceiverManager.
   */
  void refreshCpoDomData();

  /**
   * Get CPO DOM data for a specific transceiver.
   */
  CpoDomData getCpoDomData(TransceiverID tcvrId) const;

  /**
   * Get CPO DOM data for all CPO-enabled transceivers.
   */
  std::map<TransceiverID, CpoDomData> getAllCpoDomData() const;

  /**
   * Check if a transceiver is a CPO module (CPO_OE or ELSFP type).
   */
  bool isCpoModule(TransceiverID tcvrId) const;

  /**
   * Set the DOM polling interval.
   */
  void setDomPollingInterval(std::chrono::milliseconds interval);

 private:
  TransceiverManager* tcvrMgr_{nullptr};
  std::chrono::milliseconds domPollingInterval_{1000};
  mutable std::mutex cpoDomMutex_;
  std::map<TransceiverID, CpoDomData> cpoDomCache_;
};

} // namespace fboss
} // namespace facebook
