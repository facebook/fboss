// Copyright 2004-present Facebook. All Rights Reserved.

#include <gflags/gflags.h>
#include "fboss/agent/FbossInit.h"
#include "fboss/fsdb/server/Server.h"

DEFINE_bool(memoryProfiling, false, "Whether to enable memory profiling");

namespace facebook::fboss::fsdb {

int fsdbMain(int argc, char** argv) {
  setVersionString();
  auto fsdbConfig = parseConfig(argc, argv);
  initFlagDefaults(fsdbConfig->getThrift().get_defaultCommandLineArgs());

  if (FLAGS_memoryProfiling) {
    setenv("MALLOC_CONF", "prof:true", 1); // Enable heap profile
  }
  facebook::fboss::fbossInit(argc, argv);

  auto handler = createThriftHandler(fsdbConfig);
  auto server = createThriftServer(fsdbConfig, handler);
  startThriftServer(server, handler);
  return 0;
}

} // namespace facebook::fboss::fsdb

int main(int argc, char** argv) {
  return facebook::fboss::fsdb::fsdbMain(argc, argv);
}
