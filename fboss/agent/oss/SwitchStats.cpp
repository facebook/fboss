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

} // namespace facebook::fboss
