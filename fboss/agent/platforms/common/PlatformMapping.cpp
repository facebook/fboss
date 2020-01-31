/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/PlatformMapping.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook {
namespace fboss {
PlatformMapping::PlatformMapping(const std::string& jsonPlatformMappingStr) {
  auto mapping =
      apache::thrift::SimpleJSONSerializer::deserialize<cfg::PlatformMapping>(
          jsonPlatformMappingStr);
  platformPorts_ = std::move(mapping.ports);
  supportedProfiles_ = std::move(mapping.supportedProfiles);
  chips_ = std::move(mapping.chips);
}
} // namespace fboss
} // namespace facebook
