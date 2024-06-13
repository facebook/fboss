// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/Range.h>
#include "fboss/lib/if/gen-cpp2/fboss_common_types.h"

namespace facebook::fboss {

inline constexpr folly::StringPiece kPortMappingMontblancCsv{
    "Montblanc_BspMapping.csv"};
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

class Parser {
 public:
  static std::string getNameFor(PlatformType platform);

 private:
};

} // namespace facebook::fboss
