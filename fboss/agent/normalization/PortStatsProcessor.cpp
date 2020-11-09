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
#include "fboss/agent/normalization/TransformHandler.h"

namespace facebook::fboss::normalization {

PortStatsProcessor::PortStatsProcessor(TransformHandler* transformHandler)
    : transformHandler_(transformHandler) {}

void PortStatsProcessor::processStats(
    const folly::F14FastMap<std::string, HwPortStats>& hwStatsMap) {
  for (auto& [portName, hwPortStats] : hwStatsMap) {
    process(
        portName,
        "input_bps",
        StatTimestamp(*hwPortStats.timestamp__ref()),
        *hwPortStats.inBytes__ref(),
        TransformType::BPS);
    process(
        portName,
        "output_bps",
        StatTimestamp(*hwPortStats.timestamp__ref()),
        *hwPortStats.outBytes__ref(),
        TransformType::BPS);
    process(
        portName,
        "total_input_discards",
        StatTimestamp(*hwPortStats.timestamp__ref()),
        *hwPortStats.inDiscards__ref(),
        TransformType::RATE);
    process(
        portName,
        "total_output_discards",
        StatTimestamp(*hwPortStats.timestamp__ref()),
        *hwPortStats.outDiscards__ref(),
        TransformType::RATE);
  }
}

void PortStatsProcessor::process(
    const std::string& portName,
    const std::string& normalizedPropertyName,
    StatTimestamp propertyTimestamp,
    int64_t propertyValue,
    TransformType type) {
  std::optional<double> transformedValue;
  switch (type) {
    case TransformType::RATE:
      transformedValue = transformHandler_->rate(
          portName, normalizedPropertyName, propertyTimestamp, propertyValue);
      break;
    case TransformType::BPS:
      transformedValue = transformHandler_->bps(
          portName, normalizedPropertyName, propertyTimestamp, propertyValue);
      break;
  }

  if (transformedValue) {
    // TODO: create formatted ODS counter and publish it
  }
}

} // namespace facebook::fboss::normalization
