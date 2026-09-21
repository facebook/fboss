#pragma once

#include <folly/logging/AsyncFileWriter.h>
#include <folly/logging/xlog.h>
#include <map>
#include <string>
#include "folly/Singleton.h"

enum class SnapshotLogSource { WEDGE_AGENT, QSFP_SERVICE, SNAPSHOT_AGENT };

class AsyncFileWriterFactory {
 public:
  static std::shared_ptr<folly::AsyncFileWriter> getLogger(
      SnapshotLogSource source);
};
