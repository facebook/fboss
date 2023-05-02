// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook {
namespace fboss {

// Parses BspPlatformMappingThrift and provides APIs for accessing various
// mappings
class BspPlatformMapping {
 public:
  explicit BspPlatformMapping(BspPlatformMappingThrift bspMapping);

  const BspTransceiverMapping& getTcvrMapping(int tcvrID) const;
  std::map<int, BspPimMapping> getPimMappings() const;
  int getPimIDFromTcvrID(int tcvrID) const;
  int numTransceivers() const;
  int numPims() const;
  int getLedId(int tcvrId, int tcvrLaneId) const;
  LedMapping getLedMapping(int pimId, int ledId) const;

 private:
  BspPlatformMappingThrift bspMapping_;
  std::unordered_map<int, int> tcvrToPimMapping_;
  std::unordered_map<int, BspTransceiverMapping> tcvrMapping_;
};

} // namespace fboss
} // namespace facebook
