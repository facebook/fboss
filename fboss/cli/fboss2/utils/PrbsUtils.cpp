// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/utils/PrbsUtils.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace facebook::fboss {

std::vector<phy::PrbsComponent> prbsComponents(
    const std::vector<std::string>& components,
    bool returnAllIfEmpty) {
  if (components.empty() && returnAllIfEmpty) {
    // If no components are requested, return all
    return {
        phy::PrbsComponent::ASIC,
        phy::PrbsComponent::GB_SYSTEM,
        phy::PrbsComponent::GB_LINE,
        phy::PrbsComponent::TRANSCEIVER_SYSTEM,
        phy::PrbsComponent::TRANSCEIVER_LINE};
  }
  std::vector<phy::PrbsComponent> prbsComps;
  for (const auto& comp : components) {
    if (comp == "asic") {
      prbsComps.push_back(phy::PrbsComponent::ASIC);
    } else if (comp == "xphy_system") {
      prbsComps.push_back(phy::PrbsComponent::GB_SYSTEM);
    } else if (comp == "xphy_line") {
      prbsComps.push_back(phy::PrbsComponent::GB_LINE);
    } else if (comp == "transceiver_system") {
      prbsComps.push_back(phy::PrbsComponent::TRANSCEIVER_SYSTEM);
    } else if (comp == "transceiver_line") {
      prbsComps.push_back(phy::PrbsComponent::TRANSCEIVER_LINE);
    } else {
      throw std::runtime_error("Unsupported component: " + comp);
    }
  }
  return prbsComps;
}

} // namespace facebook::fboss
