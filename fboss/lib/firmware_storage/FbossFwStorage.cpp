// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/firmware_storage/FbossFwStorage.h"
#include <boost/filesystem/operations.hpp>

DEFINE_string(
    default_firmware_manifest,
    "/lib/firmware/fboss/fboss_firmware.yaml",
    "Firmware manifest file name (with path)");

namespace facebook::fboss {

/*
 * FbossFwStorage::initStorage
 *
 * This function parses the config file (provided in yaml format) which
 * contains the products and the corresponding firmware information. It
 * then creates FbossFwStorage object using root level Yaml node. This root
 * node will be used later by this object to parse the Yaml file
 */
FbossFwStorage FbossFwStorage::initStorage(std::string fwConfigFile) {
  YAML::Node fwInfoRoot;

  // Create a root level YAML::Node by parsing the config yaml file
  try {
    fwInfoRoot = YAML::LoadFile(fwConfigFile);
  } catch (const YAML::Exception& e) {
    XLOG(ERR) << "Bad config yaml file " << fwConfigFile << " " << e.what();
    throw;
  }

  // Get the firmware directory name from fwConfigFile absolute path.
  // Three cases: xyz.yaml = '.', /xyz.yaml = '/', /x/y/abc.yaml = '/x/y/'
  boost::filesystem::path p(fwConfigFile);
  boost::filesystem::path firmwareDir = p.parent_path();
  std::string fwDir = firmwareDir.string() + "/";
  XLOG(DBG2) << "Firmware directory is " << fwDir;

  // Create and return the FbossFwStorage object using that root Yaml node
  return FbossFwStorage(fwInfoRoot, fwDir);
}

/*
 * FbossFwStorage
 *
 * Constructor. This takes YAML root node of the YAML file as input. It parses
 * the file and creates a local map of product/module name to firmware record.
 * Later any call to getFirmware() will read this mapping, create the
 * FbossFirmware object and return it
 */
FbossFwStorage::FbossFwStorage(const YAML::Node& fwInfoRoot, std::string fwDir)
    : firmwareDir_(std::move(fwDir)) {
  std::string prodName;
  std::string verName;
  std::string md5sum;
  std::string fileName;

  // Check all the firmware records in the yaml file, the records are per
  // per module/product
  for (auto firmwareRecord : fwInfoRoot) {
    struct FwIdentifier fwRec;
    YAML::Node nameInfo;

    try {
      nameInfo = firmwareRecord["name"];
    } catch (const YAML::Exception&) {
      throw FbossFirmwareError("Bad Yaml file format");
    }
    if (!nameInfo.IsScalar()) {
      XLOG(ERR) << "Bad Yaml file format";
      throw FbossFirmwareError("Bad Yaml file format");
    }

    // Check if we get the right product/module name
    prodName = nameInfo.as<std::string>();
    fwRec.prodName = prodName;

    YAML::Node descr;

    try {
      descr = firmwareRecord["description"];
      fwRec.description = descr.as<std::string>();
    } catch (const YAML::Exception&) {
      XLOG(ERR) << "Description of firmware not present for prod " << prodName;
    }

    // Get the firmware records of all the firmware versions supported by this
    // module
    const YAML::Node& versionsInfo = firmwareRecord["versions"];

    // For each firmware version record
    for (auto& versionRecord : versionsInfo) {
      struct FbossFirmware::FwAttributes versionRec;
      std::unordered_map<std::string, std::string> props;

      try {
        const YAML::Node& verNameInfo = versionRecord["version"];
        verName = verNameInfo.as<std::string>();
        versionRec.version = verName;
      } catch (const YAML::Exception&) {
        throw FbossFirmwareError("Bad Yaml format: version is not mentioned");
      }

      // Get md5 checksum
      try {
        const YAML::Node& md5Info = versionRecord["md5sum"];
        md5sum = md5Info.as<std::string>();
        versionRec.md5Checksum = md5sum;
      } catch (const YAML::Exception&) {
        throw FbossFirmwareError(
            "Bad Yaml format: md5 checksum is not mentioned");
      }

      // Get image file name
      try {
        const YAML::Node& fileInfo = versionRecord["file"];
        fileName = firmwareDir_ + fileInfo.as<std::string>();
        versionRec.filename = fileName;
      } catch (const YAML::Exception&) {
        throw FbossFirmwareError(
            "Bad Yaml format: image filename is not mentioned");
      }

      // Get the properties subtree
      const YAML::Node& propertiesInfo = versionRecord["properties"];

      // Check all the properties
      // The properties are list of map
      for (auto& propertyRec : propertiesInfo) {
        // This  is one list element, access the map elements of it
        try {
          for (auto& propMap : propertyRec) {
            // Accessing map elements of the list
            const YAML::Node& key = propMap.first;
            std::string keyStr = key.as<std::string>();
            const YAML::Node& val = propMap.second;
            std::string valStr = val.as<std::string>();
            props[keyStr] = valStr;
          }
        } catch (const YAML::Exception& e) {
          XLOG(ERR)
              << "Yaml format: properties (ie: msa_password) not mentioned "
              << "for product " << prodName << " version " << verName;
          XLOG(ERR) << e.what();
        }
      }

      versionRec.properties = props;
      fwRec.versionRecord[verName] = versionRec;
    }
    firmwareRecord_[prodName] = std::move(fwRec);
  }
}

/*
 * FbossFwStorage::getFwVersionList
 *
 * This function takes the module name, looks for all the firmware versions
 * supported for this module and returns the strcuture FwIdentifier containing
 * the list of all these supported firmware versions.
 */
const struct FbossFwStorage::FwIdentifier& FbossFwStorage::getFwVersionList(
    const std::string& name) const {
  // Check if the product name exist in firmware records
  auto fwRecIt = firmwareRecord_.find(name);
  if (fwRecIt == firmwareRecord_.end()) {
    throw FbossFirmwareError("Yaml file format, product not found");
  }

  return fwRecIt->second;
}

/*
 * FbossFwStorage::getFirmware
 *
 * For a given module name and firmware version, return the FbossFirmware
 * object. This function will find out the correct firmware version record
 * based on local mapping from product/module id to firmware record and the
 * firmware_record.version id to version record. This finally creates
 * FbossFirmware object and return it
 */
std::unique_ptr<FbossFirmware> FbossFwStorage::getFirmware(
    const std::string& name,
    const std::string& version) {
  std::string prodName;
  std::string verName;
  std::string md5sum;
  std::string fileName;
  std::unordered_map<std::string, std::string> props;
  struct FbossFirmware::FwAttributes fwAttr;

  // Get the module/product firmware record for this given module/product name
  auto fwRecIt = firmwareRecord_.find(name);
  if (fwRecIt == firmwareRecord_.end()) {
    throw FbossFirmwareError("Bad Yaml file format, module not found");
  }

  // Get the version record for this given firmware version id
  auto verRecIt = fwRecIt->second.versionRecord.find(version);
  if (verRecIt == fwRecIt->second.versionRecord.end()) {
    throw FbossFirmwareError("Bad Yaml file format, version info not found");
  }

  // Create FbossFirmware object and return
  return std::make_unique<FbossFirmware>(verRecIt->second);
}

} // namespace facebook::fboss
