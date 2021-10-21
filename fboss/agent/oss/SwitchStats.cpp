// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/SwitchStats.h"

namespace facebook::fboss {

// static
std::unique_ptr<SwitchStats::TLTimeseries> SwitchStats::makeTLTimeseries(
    ThreadLocalStatsMap* map,
    std::string&& key,
    fb303::ExportType exportType) {
  return std::make_unique<SwitchStats::TLTimeseries>(map, key, exportType);
}

// static
std::unique_ptr<SwitchStats::TLTimeseries> SwitchStats::makeTLTimeseries(
    ThreadLocalStatsMap* map,
    std::string&& key,
    fb303::ExportType exportType1,
    fb303::ExportType exportType2) {
  return std::make_unique<SwitchStats::TLTimeseries>(
      map, key, exportType1, exportType2);
}

// static
std::unique_ptr<SwitchStats::TLHistogram> SwitchStats::makeTLTHistogram(
    ThreadLocalStatsMap* map,
    std::string&& key,
    int64_t bucketWidth,
    int64_t min,
    int64_t max) {
  return std::make_unique<SwitchStats::TLHistogram>(
      map, key, bucketWidth, min, max);
}

// static
std::unique_ptr<SwitchStats::TLHistogram> SwitchStats::makeTLTHistogram(
    ThreadLocalStatsMap* map,
    std::string&& key,
    int64_t bucketWidth,
    int64_t min,
    int64_t max,
    fb303::ExportType exportType,
    int percentile1,
    int percentile2) {
  return std::make_unique<SwitchStats::TLHistogram>(
      map, key, bucketWidth, min, max, exportType, percentile1, percentile2);
}

} // namespace facebook::fboss
