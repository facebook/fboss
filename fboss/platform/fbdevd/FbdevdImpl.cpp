#include "fboss/platform/fbdevd/FbdevdImpl.h"

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "folly/Synchronized.h"

namespace facebook::fboss::platform::fbdevd {
using namespace facebook::fboss::platform::fbdevd;

void FbdevdImpl::initPlatformConfig() {
  std::string fbdevdConfJson = ConfigLib().getFbdevdConfig();
  apache::thrift::SimpleJSONSerializer::deserialize<FbdevdConfig>(
      fbdevdConfJson, fbdevdConfig_);

  XLOG(DBG2) << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
      fbdevdConfig_);
}

FbdevdImpl::~FbdevdImpl() = default;

} // namespace facebook::fboss::platform::fbdevd
