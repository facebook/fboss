/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/MultiPimPlatformMapping.h"

#include "fboss/agent/FbossError.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook {
namespace fboss {
MultiPimPlatformMapping::MultiPimPlatformMapping(
    const std::string& jsonPlatformMappingStr)
    : PlatformMapping(jsonPlatformMappingStr) {
  for (auto& port : platformPorts_) {
    int portPimID = getPimID(port.second);

    if (pims_.find(portPimID) == pims_.end()) {
      pims_[portPimID] = std::make_unique<PlatformMapping>();
    }

    pims_[portPimID]->setPlatformPort(port.first, port.second);

    const auto& portChips = utility::getDataPlanePhyChips(port.second, chips_);
    for (auto itChip : portChips) {
      pims_[portPimID]->setChip(itChip.first, itChip.second);
    }

    for (auto& portProfile : *port.second.supportedProfiles_ref()) {
      if (auto platformProfile =
              getPortProfileConfig(PlatformPortProfileConfigMatcher(
                  portProfile.first, PimID(portPimID)))) {
        cfg::PlatformPortProfileConfigEntry configEntry;
        cfg::PlatformPortConfigFactor factor;
        factor.profileID_ref() = portProfile.first;
        factor.pimIDs_ref() = {portPimID};
        configEntry.profile_ref() = platformProfile.value();
        configEntry.factor_ref() = factor;
        pims_[portPimID]->mergePlatformSupportedProfile(configEntry);
      } else {
        throw FbossError(
            "Port:",
            *port.second.mapping_ref()->name_ref(),
            " uses unsupported platform profile:",
            apache::thrift::util::enumNameSafe(portProfile.first));
      }
    }

    auto portConfigOverrides = getPortConfigOverrides(port.first);
    pims_[portPimID]->mergePortConfigOverrides(port.first, portConfigOverrides);
  }
}

PlatformMapping* MultiPimPlatformMapping::getPimPlatformMapping(uint8_t pimID) {
  if (auto itPim = pims_.find(pimID); itPim != pims_.end()) {
    return itPim->second.get();
  }
  throw FbossError("Invalid pim id:", static_cast<int>(pimID));
}

std::unique_ptr<PlatformMapping>
MultiPimPlatformMapping::getPimPlatformMappingUniquePtr(uint8_t pimID) {
  if (auto itPim = pims_.find(pimID); itPim != pims_.end()) {
    return std::move(itPim->second);
  }
  throw FbossError("Invalid pim id:", static_cast<int>(pimID));
}
} // namespace fboss
} // namespace facebook
