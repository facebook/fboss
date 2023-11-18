// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/server/FsdbMain.h"
#include <gflags/gflags.h>
#include "common/base/BuildInfo.h"

int main(int argc, char** argv) {
  gflags::SetVersionString(facebook::BuildInfo::toDebugString());
  return facebook::fboss::fsdb::fsdbMain(argc, argv);
}
