/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/CmdGlobalOptions.h"

#include <folly/Singleton.h>

namespace {
struct singleton_tag_type {};
} // namespace

using facebook::fboss::CmdGlobalOptions;
static folly::Singleton<CmdGlobalOptions, singleton_tag_type>
    cmdGlobalOptions{};
std::shared_ptr<CmdGlobalOptions> CmdGlobalOptions::getInstance() {
  return cmdGlobalOptions.try_get();
}

namespace facebook::fboss {

void CmdGlobalOptions::init(CLI::App& app) {
  app.add_option("--host", hosts_, "Hostname(s) to query");
  app.add_option("-l,--loglevel", logLevel_, "Debug log level");
  app.add_option("--file", file_, "filename, queries every host in the file");

  initAdditional(app);
}

} // namespace facebook::fboss
