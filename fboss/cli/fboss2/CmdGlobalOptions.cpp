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
#include <CLI/Validators.hpp>

#include <folly/Singleton.h>
#include <folly/String.h>

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
  app.add_option("-H, --host", hosts_, "Hostname(s) to query");
  app.add_option("-l,--loglevel", logLevel_, "Debug log level");
  app.add_option("--file", file_, "filename, queries every host in the file")
      ->check(CLI::ExistingFile);
  app.add_option("--fmt", fmt_, OutputFormat::getDescription())
      ->check(OutputFormat::getValidator());
  app.add_option(
         "--agent-port", agentThriftPort_, "Agent thrift port to connect to")
      ->check(CLI::PositiveNumber);
  app.add_option(
         "--qsfp-port",
         qsfpThriftPort_,
         "QsfpService thrift port to connect to")
      ->check(CLI::PositiveNumber);
  app.add_option(
      "--color", color_, "color (no, yes => yes for tty and no for pipe)");
  app.add_option(
      "--filter",
      filters_,
      "filter list. Each filter must be in the form <key>=<value>. See specific commands for list of available filters");

  initAdditional(app);
}

std::map<std::string, std::string> CmdGlobalOptions::getFilters() const {
  std::map<std::string, std::string> parsedFilters;
  for (const auto& filter : filters_) {
    std::vector<std::string> vec;
    folly::split("=", filter, vec);
    if (vec.size() != 2) {
      throw std::runtime_error(fmt::format(
          "Filters need to be in the form <key>=value>. {} does not match",
          filter));
    }
    parsedFilters[vec[0]] = vec[1];
  }
  return parsedFilters;
}

} // namespace facebook::fboss
