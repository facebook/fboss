// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.
#include <fb303/FollyLoggingHandler.h>

#include "fboss/platform/helpers/InitCli.h"
#include "fboss/platform/showtech/Utils.h"

using namespace facebook;
using namespace facebook::fboss::platform;

int main(int argc, char** argv) {
  helpers::initCli(&argc, &argv, "showtech");

  Utils().printHostDetails();
  Utils().printFbossDetails();
  Utils().printWeutilDetails();
  Utils().printFwutilDetails();
  return 0;
}
