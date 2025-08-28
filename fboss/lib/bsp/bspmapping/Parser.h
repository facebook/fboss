// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/Range.h>
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"
#include "fboss/lib/if/gen-cpp2/fboss_common_types.h"

namespace facebook::fboss {

inline const int HEADER_OFFSET = 1;
inline const int MIN_LANE_ID = 1;
inline const int MAX_LANE_ID = 8;
inline constexpr folly::StringPiece kInputConfigPrefix{
    "fboss/lib/bsp/bspmapping/input/"};
inline constexpr folly::StringPiece kPortMappingMontblancCsv{
    "Montblanc_BspMapping.csv"};
inline constexpr folly::StringPiece kPortMappingMinipack3NCsv{
    "Minipack3N_BspMapping.csv"};
inline constexpr folly::StringPiece kPortMappingMeru400bfuCsv{
    "Meru400bfu_BspMapping.csv"};
inline constexpr folly::StringPiece kPortMappingMeru400biuCsv{
    "Meru400biu_BspMapping.csv"};
inline constexpr folly::StringPiece kPortMappingMeru800biaCsv{
    "Meru800bia_BspMapping.csv"};
inline constexpr folly::StringPiece kPortMappingMeru800bfaCsv{
    "Meru800bfa_BspMapping.csv"};
inline constexpr folly::StringPiece kPortMappingJanga800bicCsv{
    "Janga800bic_BspMapping.csv"};
inline constexpr folly::StringPiece kPortMappingTahan800bcCsv{
    "Tahan800bc_BspMapping.csv"};
inline constexpr folly::StringPiece kPortMappingMorgan800ccCsv{
    "Morgan800cc_BspMapping.csv"};
inline constexpr folly::StringPiece kPortMappingIcecube800bcCsv{
    "Icecube800bc_BspMapping.csv"};
inline constexpr folly::StringPiece kPortMappingIcetea800bcCsv{
    "Icetea800bc_BspMapping.csv"};

class Parser {
 public:
  static std::string getNameFor(PlatformType platform);
  static TransceiverConfigRow getTransceiverConfigRowFromCsvLine(
      const std::string_view& line);
  static std::vector<TransceiverConfigRow> getTransceiverConfigRowsFromCsv(
      folly::StringPiece csv);
  static BspPlatformMappingThrift getBspPlatformMappingFromCsv(
      folly::StringPiece csv);

 private:
  static std::vector<int> getTransceiverLaneIdList(
      const std::string_view& entry);
  static ResetAndPresenceAccessType getAccessCtrlTypeFromString(
      const std::string_view& entry);
  static TransceiverIOType getTransceiverIOTypeFromString(
      const std::string_view& entry);
};

} // namespace facebook::fboss
