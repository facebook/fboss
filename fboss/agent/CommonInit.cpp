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
#include <fb303/ServiceData.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/FbossInit.h"

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

std::unique_ptr<AgentConfig> fbossCommonInit(int argc, char** argv) {
  setVersionInfo();

  // Read the config and set default command line arguments
  auto config = parseConfig(argc, argv);
  initFlagDefaults(*config->thrift.defaultCommandLineArgs());

  fbossInit(argc, argv);
  // Allow the fb303 setOption() call to update the command line flag
  // settings.  This allows us to change the log levels on the fly using
  // setOption().
  fb303::fbData->setUseOptionsAsFlags(true);

  // Redirect stdin to /dev/null. This is really a extra precaution
  // we already disallow access to linux shell as a result of
  // executing thrift calls. Redirecting to /dev/null is done so that
  // if somehow a client did manage to get into the shell, the shell
  // would read EOF immediately and exit.
  if (!freopen("/dev/null", "r", stdin)) {
    XLOG(DBG2) << "Could not open /dev/null ";
  }

  // Initialize Bitsflow for agent
  initializeBitsflow();

  return config;
}

} // namespace facebook::fboss
