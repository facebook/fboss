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

#include <re2/re2.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace {
constexpr auto kFbossPortNameRegex = "eth(\\d+)/(\\d+)/1";
}

namespace facebook {
namespace fboss {
MultiPimPlatformMapping::MultiPimPlatformMapping(
    const std::string& jsonPlatformMappingStr)
    : PlatformMapping(jsonPlatformMappingStr) {
  for (auto& port : platformPorts_) {
    int portPimID = 0;
    re2::RE2 portNameRe(kFbossPortNameRegex);
    if (!re2::RE2::FullMatch(
            port.second.mapping.name, portNameRe, &portPimID)) {
      throw FbossError(
          "Invalid port name:",
          port.second.mapping.name,
          " for port id:",
          port.second.mapping.id);
    }

    if (pims_.find(portPimID) == pims_.end()) {
      pims_[portPimID] = std::make_unique<PlatformMapping>();
    }

    pims_[portPimID]->setPlatformPort(port.first, port.second);

    const auto& portChips = utility::getDataPlanePhyChips(port.second, chips_);
    for (auto itChip : portChips) {
      pims_[portPimID]->setChip(itChip.first, itChip.second);
    }

    for (auto& portProfile : port.second.supportedProfiles) {
      if (auto itProfile = supportedProfiles_.find(portProfile.first);
          itProfile != supportedProfiles_.end()) {
        pims_[portPimID]->setSupportedProfile(
            itProfile->first, itProfile->second);
      } else {
        throw FbossError(
            "Port:",
            port.second.mapping.name,
            " uses unsupported profile:",
            apache::thrift::util::enumNameSafe(portProfile.first));
      }
    }
  }
}

PlatformMapping* MultiPimPlatformMapping::getPimPlatformMapping(uint8_t pimID) {
  if (auto itPim = pims_.find(pimID); itPim != pims_.end()) {
    return itPim->second.get();
  }
  throw FbossError("Invalid pim id:", static_cast<int>(pimID));
}
} // namespace fboss
} // namespace facebook
