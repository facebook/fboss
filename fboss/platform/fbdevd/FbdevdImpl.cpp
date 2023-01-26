#include "fboss/platform/fbdevd/FbdevdImpl.h"
#include <folly/FileUtil.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <string>
#include "fboss/platform/config_lib/ConfigLib.h"
#include "folly/Synchronized.h"

namespace facebook::fboss::platform::fbdevd {
using namespace facebook::fboss::platform::fbdevd;

void FbdevdImpl::initPlatformConfig() {
  std::string fbdevdConfJson;
  // Check if conf file name is set, if not, get from config lib
  if (confFileName_.empty()) {
    fbdevdConfJson = config_lib::getFbdevdConfig();
  } else if (!folly::readFile(confFileName_.c_str(), fbdevdConfJson)) {
    throw std::runtime_error(
        "Can not find fbdevd config file: " + confFileName_);
  }

  // Clear everything before filling the structure
  fbdevdConfig_ = {};

  apache::thrift::SimpleJSONSerializer::deserialize<fbossPlatformDesc>(
      fbdevdConfJson, fbdevdConfig_);

  XLOG(DBG2) << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
      fbdevdConfig_);
}

FbdevdImpl::~FbdevdImpl() {}

} // namespace facebook::fboss::platform::fbdevd
