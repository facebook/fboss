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

#include <map>
#include <memory>
#include <string>

namespace facebook::fboss {

struct AgentConfig;

void initFlagDefaults(const std::map<std::string, std::string>& defaults);
std::unique_ptr<AgentConfig> parseConfig(int argc, char** argv);
void fbossFinalize();
void setVersionInfo();
void initializeBitsflow();
std::unique_ptr<AgentConfig> fbossCommonInit(int argc, char** argv);

} // namespace facebook::fboss
