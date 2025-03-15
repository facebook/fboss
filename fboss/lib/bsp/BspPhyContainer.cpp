// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspPhyContainer.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook {
namespace fboss {

BspPhyContainer::BspPhyContainer(
    int pimID,
    BspPhyMapping& phyMapping,
    BspPhyIO* phyIO)
    : phyMapping_(phyMapping), phyIO_(phyIO) {
  pimID_ = pimID;
  phyID_ = *phyMapping.phyId();
  // phyIO_ = std::make_unique<BspPhyIO>(pimID_, phyMapping);
}

} // namespace fboss
} // namespace facebook
