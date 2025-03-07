/*
 *  Copyright (c) 2023-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiMinipack3NPlatform.h"
#include "fboss/agent/platforms/common/minipack3n/Minipack3NPlatformMapping.h"
#include "fboss/agent/platforms/sai/SaiYangraPlatform.h"

namespace facebook::fboss {

SaiMinipack3NPlatform::SaiMinipack3NPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiYangraPlatform(
          std::move(productInfo),
          localMac,
          platformMappingStr.empty()
              ? std::make_unique<Minipack3NPlatformMapping>()
              : std::make_unique<Minipack3NPlatformMapping>(
                    platformMappingStr)) {}

const std::unordered_map<std::string, std::string>
SaiMinipack3NPlatform::getSaiProfileVendorExtensionValues() const {
  auto kv_map = SaiYangraPlatform::getSaiProfileVendorExtensionValues();
  auto itr = kv_map.find("SAI_KEY_AUTO_POPULATE_PORT_DB");
  if (itr != kv_map.end()) {
    // no auto discovery of ports for minipack3n
    kv_map.erase(itr);
  }
  kv_map.insert(std::make_pair("SAI_INDEPENDENT_MODULE_MODE", "2"));
  return kv_map;
}

SaiMinipack3NPlatform::~SaiMinipack3NPlatform() = default;

std::string SaiMinipack3NPlatform::getHwConfig() {
  // TODO: remove this XML file processing once XML file is embeded in agent
  // config
  std::string xml_filename =
      *config()->thrift.platform()->get_chip().get_asic().config();
  std::string base_filename =
      xml_filename.substr(0, xml_filename.find(".xml") + 4);
  std::ifstream xml_file(base_filename);
  std::string xml_config(
      (std::istreambuf_iterator<char>(xml_file)),
      std::istreambuf_iterator<char>());
  // std::cout << "Read config from: " << xml_filename << std::endl;
  // std::cout << "Content:" << std::endl << xml_config << std::endl;
  if (xml_filename.find(";disable_lb_filter") != std::string::npos) {
    std::string keyTag = "</issu-enabled>";
    std::string newKeyTag = "<lb_filter_disable>1</lb_filter_disable>";
    int indentSpaces = 8;

    size_t pos = xml_config.find(keyTag);
    if (pos != std::string::npos) {
      xml_config.insert(
          pos + keyTag.length(), std::string(indentSpaces, ' ') + newKeyTag);
    } else {
      std::cerr << "Tag " << keyTag << " not found in XML." << std::endl;
    }
  }
  return xml_config;
}
} // namespace facebook::fboss
