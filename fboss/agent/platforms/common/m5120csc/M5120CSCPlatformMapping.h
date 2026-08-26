/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/platforms/common/PlatformMapping.h"

namespace facebook::fboss {

class M5120CSCPlatformMapping : public PlatformMapping {
 public:
  M5120CSCPlatformMapping();
  explicit M5120CSCPlatformMapping(const std::string& platformMappingStr);
  ~M5120CSCPlatformMapping() = default;

 private:
  // Forbidden copy constructor and assignment operator
  M5120CSCPlatformMapping(M5120CSCPlatformMapping const&) = delete;
  M5120CSCPlatformMapping& operator=(M5120CSCPlatformMapping const&) = delete;
  M5120CSCPlatformMapping(M5120CSCPlatformMapping&&) = delete;
  M5120CSCPlatformMapping& operator=(M5120CSCPlatformMapping&&) = delete;
};
} // namespace facebook::fboss
