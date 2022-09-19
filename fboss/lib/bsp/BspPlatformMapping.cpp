// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook {
namespace fboss {

BspPlatformMapping::BspPlatformMapping(BspPlatformMappingThrift bspMapping)
    : bspMapping_(bspMapping) {
  for (auto pim : *bspMapping_.pimMapping()) {
    auto pimID = pim.first;
    auto pimInfo = pim.second;
    for (auto tcvr : *pimInfo.tcvrMapping()) {
      auto tcvrID = tcvr.first;
      auto tcvrInfo = tcvr.second;
      // Confirm that we haven't seen the same tcvrID before
      CHECK(tcvrToPimMapping_.find(tcvrID) == tcvrToPimMapping_.end());
      tcvrToPimMapping_[tcvrID] = pimID;
      tcvrMapping_[tcvrID] = tcvrInfo;
    }
  }
}

int BspPlatformMapping::getPimIDFromTcvrID(int tcvrID) const {
  CHECK(tcvrToPimMapping_.find(tcvrID) != tcvrToPimMapping_.end());
  return tcvrToPimMapping_.at(tcvrID);
}

const BspTransceiverMapping& BspPlatformMapping::getTcvrMapping(
    int tcvrID) const {
  CHECK(tcvrMapping_.find(tcvrID) != tcvrMapping_.end());
  return tcvrMapping_.at(tcvrID);
}

std::map<int, BspPimMapping> BspPlatformMapping::getPimMappings() const {
  return *bspMapping_.pimMapping();
}

int BspPlatformMapping::numTransceivers() const {
  return tcvrMapping_.size();
}

int BspPlatformMapping::numPims() const {
  return bspMapping_.pimMapping()->size();
}

} // namespace fboss
} // namespace facebook
