/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/normalization/PortStatsProcessor.h"
#include "fboss/agent/normalization/StatsExporter.h"
#include "fboss/agent/normalization/TransformHandler.h"
#include "folly/logging/xlog.h"

namespace facebook::fboss::normalization {

PortStatsProcessor::PortStatsProcessor(
    TransformHandler* transformHandler,
    StatsExporter* statsExporter,
    CounterTagManager* counterTagManager)
    : transformHandler_(transformHandler),
      statsExporter_(statsExporter),
      counterTagManager_(counterTagManager) {}

void PortStatsProcessor::processStats(
    const folly::F14FastMap<std::string, HwPortStats>& hwStatsMap) {
  for (auto& [portName, hwPortStats] : hwStatsMap) {
    process(
        portName,
        "input_bps",
        StatTimestamp(*hwPortStats.timestamp__ref()),
        *hwPortStats.inBytes__ref(),
        TransformType::BPS,
        60);
    process(
        portName,
        "output_bps",
        StatTimestamp(*hwPortStats.timestamp__ref()),
        *hwPortStats.outBytes__ref(),
        TransformType::BPS,
        60);
    process(
        portName,
        "total_input_discards",
        StatTimestamp(*hwPortStats.timestamp__ref()),
        *hwPortStats.inDiscards__ref(),
        TransformType::RATE,
        60);
    process(
        portName,
        "total_output_discards",
        StatTimestamp(*hwPortStats.timestamp__ref()),
        *hwPortStats.outDiscards__ref(),
        TransformType::RATE,
        60);
  }

  statsExporter_->flushCounters();
}

void PortStatsProcessor::process(
    const std::string& portName,
    const std::string& normalizedPropertyName,
    StatTimestamp propertyTimestamp,
    int64_t propertyValue,
    TransformType type,
    int32_t processIntervalSec) {
  std::optional<double> transformedValue;
  switch (type) {
    case TransformType::RATE:
      transformedValue = transformHandler_->rate(
          portName,
          normalizedPropertyName,
          propertyTimestamp,
          propertyValue,
          processIntervalSec);
      break;
    case TransformType::BPS:
      transformedValue = transformHandler_->bps(
          portName,
          normalizedPropertyName,
          propertyTimestamp,
          propertyValue,
          processIntervalSec);
      break;
  }

  if (transformedValue) {
    auto counterTags = counterTagManager_->getCounterTags(portName);
    // create formatted ODS counter and publish it
    statsExporter_->publishPortStats(
        portName,
        normalizedPropertyName,
        propertyTimestamp,
        *transformedValue,
        processIntervalSec,
        std::move(counterTags));
  }
}

} // namespace facebook::fboss::normalization
