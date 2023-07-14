/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/CommonInit.h"
#include "fboss/agent/AgentConfig.h"

#include <gflags/gflags.h>
#include <cstdio>
#include <iostream>

using std::shared_ptr;
using std::string;
using std::unique_ptr;

namespace facebook::fboss {
void initFlagDefaults(const std::map<std::string, std::string>& defaults) {
  for (auto item : defaults) {
    // logging not initialized yet, need to use std::cerr
    std::cerr << "Overriding default flag from config: " << item.first.c_str()
              << "=" << item.second.c_str() << std::endl;
    gflags::SetCommandLineOptionWithMode(
        item.first.c_str(), item.second.c_str(), gflags::SET_FLAGS_DEFAULT);
  }
}

std::unique_ptr<AgentConfig> parseConfig(int argc, char** argv) {
  // one pass over flags, but don't clear argc/argv. We only do this
  // to extract the 'config' arg.
  gflags::ParseCommandLineFlags(&argc, &argv, false);
  return AgentConfig::fromDefaultFile();
}

} // namespace facebook::fboss
