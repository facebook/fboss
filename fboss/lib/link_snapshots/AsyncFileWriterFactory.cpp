#include "fboss/lib/link_snapshots/AsyncFileWriterFactory.h"
#include <boost/filesystem.hpp>
#include <map>
#include <sstream>
#include <string>

folly::Singleton<
    std::map<SnapshotLogSource, std::shared_ptr<folly::AsyncFileWriter>>>
    asyncLoggers;

std::shared_ptr<folly::AsyncFileWriter> AsyncFileWriterFactory::getLogger(
    SnapshotLogSource type) {
  auto loggerMap = asyncLoggers.try_get();
  auto it = loggerMap->find(type);
  if (it == loggerMap->end()) {
    std::string logPath;
    boost::filesystem::path dirPath("/var/facebook/logs/fboss/");
    if (boost::filesystem::exists(dirPath)) {
      logPath = dirPath.string();
    } else {
      logPath = "/tmp/";
    }
    std::string logSuffix;
    switch (type) {
      case SnapshotLogSource::WEDGE_AGENT:
        logSuffix = "wedge_agent_snapshots.log";
        break;
      case SnapshotLogSource::QSFP_SERVICE:
        logSuffix = "qsfp_service_snapshots.log";
        break;
      case SnapshotLogSource::SNAPSHOT_AGENT:
        logSuffix = "snapshot_agent_snapshots.log";
        break;
    }
    // Set the log path to be the directory plus the suffix
    logPath += logSuffix;
    auto logger = std::make_shared<folly::AsyncFileWriter>(logPath);
    loggerMap->emplace(type, logger);
    return logger;
  }
  return it->second;
}
