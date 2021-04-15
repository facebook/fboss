/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/CLI11/CLI.hpp"

#include <folly/init/Init.h>

namespace facebook::fboss {

int cliMain(int argc, char* argv[]) {
   int one = 1;
   folly::init(&one, &argv, /* removeFlags = */ false);

   CLI::App app{"FBOSS CLI"};

   app.require_subcommand();

   CLI11_PARSE(app, argc, argv);

   return 0;
}

} // namespace facebook::fboss

int main(int argc, char* argv[]) {
  return facebook::fboss::cliMain(argc, argv);
}
