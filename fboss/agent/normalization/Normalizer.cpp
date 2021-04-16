/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/normalization/Normalizer.h"

#include <folly/Singleton.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/normalization/PortStatsProcessor.h"

DEFINE_bool(
    enable_counter_normalization,
    false,
    "Load the agent with counter normalization support enabled");
DEFINE_string(
    default_device_name,
    "unknown_device",
    "default device name used for counter normalization");

namespace facebook::fboss {
namespace {
folly::Singleton<Normalizer> theInstance;

void print(const folly::F14FastMap<std::string, HwPortStats>& hwStatsMap) {
  for (auto& [portName, hwPortStats] : hwStatsMap) {
    XLOGF(
        DBG6,
        "ts: {}, port name: {}, inBytes: {}, outBytes: {}",
        *hwPortStats.timestamp__ref(),
        portName,
        *hwPortStats.inBytes__ref(),
        *hwPortStats.outBytes__ref());
  }
}

} // namespace

std::shared_ptr<Normalizer> Normalizer::getInstance() {
  return theInstance.try_get();
}

void Normalizer::processStats(
    const folly::F14FastMap<std::string, HwPortStats>& hwStatsMap) {
  if (!FLAGS_enable_counter_normalization) {
    return;
  }

  if (XLOG_IS_ON(DBG6)) {
    print(hwStatsMap);
  }
  normalization::PortStatsProcessor processor(
      transformHandler_.get(), statsExporter_.get(), counterTagManager_.get());
  processor.processStats(hwStatsMap);

  XLOG(DBG5) << "normalizer processed stats";
}

void Normalizer::processLinkStateChange(
    const std::string& portName,
    bool isUp) {
  if (!FLAGS_enable_counter_normalization) {
    return;
  }
  XLOGF(
      DBG6, "port {} link state changed to {}", portName, isUp ? "UP" : "DOWN");
}

void Normalizer::reloadCounterTags(const cfg::SwitchConfig& curConfig) {
  counterTagManager_->reloadCounterTags(curConfig);
}

} // namespace facebook::fboss
