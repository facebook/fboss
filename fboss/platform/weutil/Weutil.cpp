// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/platform/weutil/Weutil.h"

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <ios>
#include <string>

#include "fboss/lib/platforms/PlatformMode.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"
#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/weutil/WeutilDarwin.h"
#include "fboss/platform/weutil/WeutilImpl.h"
#include "fboss/platform/weutil/if/gen-cpp2/weutil_config_types.h"

namespace facebook::fboss::platform {

PlainWeutilConfig parseConfig(const std::string& configFile) {
  // In this method, we get the config file from ConfigLib,
  // then translate that thrift structure to a plain structure.
  // The reason of this is because we will share the same WeutilImpl
  // in the BMC codebase, where we do not have access to Thrift/Folly.

  // This is thrift based config datastructure,
  // to be translated as the plain config above
  weutil_config::WeutilConfig thriftConfig;

  // And, the following is struct based config datastructure, that
  // will be used inside WeutilImpl
  PlainWeutilConfig config;

  // First, get the config file as string (JSON format)
  std::string weutilConfigJson;
  // Check if conf file name is set, if not, set the default name
  if (configFile.empty()) {
    XLOG(INFO) << "No config file was provided. Inferring from config_lib";
    weutilConfigJson = ConfigLib().getWeutilConfig();
  } else {
    XLOG(INFO) << "Using config file: " << configFile;
    if (!folly::readFile(configFile.c_str(), weutilConfigJson)) {
      throw std::runtime_error(
          "Can not find weutil config file: " + configFile);
    }
  }

  // Secondly, deserialize the JSON string to thrift struct
  apache::thrift::SimpleJSONSerializer::deserialize<
      weutil_config::WeutilConfig>(weutilConfigJson, thriftConfig);
  XLOG(INFO) << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
      thriftConfig);

  // Finally, translate the thrift struct to a plain struct
  config.chassisEeprom = *thriftConfig.chassisEepromName();
  for (auto& [eepromName, eepromConfig] : *thriftConfig.fruEepromList()) {
    std::string fruName = eepromName;
    std::transform(fruName.begin(), fruName.end(), fruName.begin(), ::tolower);
    std::string path = *eepromConfig.path();
    int offset = *eepromConfig.offset();
    int version = int(*eepromConfig.idEepromFormatVer());
    config.configEntry[fruName] = {path, offset, version};
  }

  // Then return the plain struct.
  return config;
}

std::unique_ptr<WeutilInterface> get_plat_weutil(
    const std::string& eeprom,
    const std::string& configFile) {
  facebook::fboss::PlatformProductInfo prodInfo{FLAGS_fruid_filepath};

  prodInfo.initialize();

  if (prodInfo.getType() == PlatformType::PLATFORM_DARWIN) {
    std::unique_ptr<WeutilDarwin> pDarwinIntf;
    pDarwinIntf = std::make_unique<WeutilDarwin>(eeprom);
    if (pDarwinIntf->getEepromPath()) {
      return std::move(pDarwinIntf);
    } else {
      return nullptr;
    }
  } else {
    PlainWeutilConfig config = parseConfig(configFile);
    std::unique_ptr<WeutilImpl> pWeutilImpl =
        std::make_unique<WeutilImpl>(eeprom, config);
    if (pWeutilImpl->getEepromPath()) {
      return std::move(pWeutilImpl);
    } else {
      return nullptr;
    }
  }

  XLOG(INFO) << "The platform (" << toString(prodInfo.getType())
             << ") is not supported" << std::endl;
  return nullptr;
}

std::unique_ptr<WeutilInterface> get_meta_eeprom_handler(std::string path) {
  // Absolute path is given. No config needed. Use dummy config instead.
  PlainWeutilConfig dummyConfig;
  // Note that we pass path as the eeprom name, since we will skip the
  // eeprom name to the path translation (done by calling verify option method.)
  std::unique_ptr<WeutilImpl> pWeutilImpl =
      std::make_unique<WeutilImpl>(path, dummyConfig);
  return std::move(pWeutilImpl);
}

} // namespace facebook::fboss::platform
