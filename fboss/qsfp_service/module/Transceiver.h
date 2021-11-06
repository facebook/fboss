/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once
#include <cstdint>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <folly/futures/Future.h>

namespace facebook {
namespace fboss {

/* Virtual class to handle the different transceivers our equipment is likely
 * to support.  This supports, for now, QSFP and SFP.
 */

struct TransceiverID;

class Transceiver {
 public:
  Transceiver() {}
  virtual ~Transceiver() {}

  /*
   * Transceiver type (SFP, QSFP)
   */
  virtual TransceiverType type() const = 0;

  virtual TransceiverID getID() const = 0;

  /*
   * Return the spec this transceiver follows.
   */
  virtual TransceiverManagementInterface managementInterface() const = 0;

  /*
   * Returns if the SFP is present or not
   */
  virtual bool detectPresence() = 0;

  /*
   * Check if the transceiver is present or not and refresh data.
   */
  virtual void refresh() = 0;
  virtual folly::Future<folly::Unit> futureRefresh() = 0;

  /*
   * Return all of the transceiver information
   */
  virtual TransceiverInfo getTransceiverInfo() = 0;

  /*
   * Return raw page data from the qsfp DOM
   */
  virtual RawDOMData getRawDOMData() = 0;

  /*
   * Return a union of two spec formated data from the qsfp DOM
   */
  virtual DOMDataUnion getDOMDataUnion() = 0;

  /*
   * Program Transceiver based on speed
   * This new function will combine current customizeTransceiver() and
   * configureModule() from QsfpModule.
   * TODO(joseph5wu) Will deprecate configureModule() and customizeTransceiver()
   * once we switch to use the new state machine.
   */
  virtual void programTransceiver(cfg::PortSpeed speed) = 0;

  /*
   * Set speed specific settings for the transceiver
   */
  virtual void customizeTransceiver(cfg::PortSpeed speed) = 0;

  /*
   * Register that a logical port that is part of this transceiver has changed.
   */
  virtual void transceiverPortsChanged(
      const std::map<uint32_t, PortStatus>& ports) = 0;

  /*
   * Perform raw register read on a specific transceiver
   */
  virtual std::unique_ptr<IOBuf> readTransceiver(
      TransceiverIOParameters param) = 0;
  /*
   * Future version of readTransceiver()
   */
  virtual folly::Future<std::pair<int32_t, std::unique_ptr<IOBuf>>>
  futureReadTransceiver(TransceiverIOParameters param) = 0;

  /*
   * Perform raw register write on a specific transceiver
   */
  virtual bool writeTransceiver(
      TransceiverIOParameters param,
      uint8_t data) = 0;
  /*
   * Future version of writeTransceiver()
   */
  virtual folly::Future<std::pair<int32_t, bool>> futureWriteTransceiver(
      TransceiverIOParameters param,
      uint8_t data) = 0;

  /*
   * return the cached signal flags and clear it after the read like an clear
   * on read register.
   */
  virtual SignalFlags readAndClearCachedSignalFlags() = 0;

  /*
   * return the cached media lane signals and clear it after the read like an
   * clear on read register.
   */
  virtual std::map<int, MediaLaneSignals>
  readAndClearCachedMediaLaneSignals() = 0;

 private:
  // no copy or assignment
  Transceiver(Transceiver const&) = delete;
  Transceiver& operator=(Transceiver const&) = delete;
};

} // namespace fboss
} // namespace facebook
