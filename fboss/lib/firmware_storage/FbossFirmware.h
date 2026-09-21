// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/io/Cursor.h>

namespace facebook::fboss {

/*
 * FwStorageError
 * Error class for Firmware storage library
 */
class FbossFirmwareError : public std::exception {
 public:
  explicit FbossFirmwareError(const std::string& what) : what_(what) {}

  const char* what() const noexcept override {
    return what_.c_str();
  }

 private:
  const std::string what_;
};

/*
 * FbossFirmware
 *
 * This class represents a particular firmware. A firmware is identified by
 * the module name and firmware version id. The constructor takes a structure
 * called FwAttributes which contains the product/module name, description,
 * firmware version string, firmware file name, md5 checksum value and
 * properties (like password). These values are typically provided by caller
 * after extracting from a Yaml file.
 */
class FbossFirmware {
 public:
  // Record of one firmware version. This contains product/module name,
  // description, firmware version string, image filename, md5 and map of
  // the properties like msa_password
  struct FwAttributes {
    std::string prodName;
    std::string description;
    std::string version;
    std::string filename;
    std::string md5Checksum;
    std::unordered_map<std::string, std::string> properties;
  };

  // Constructor using module name, firmware version number, firmware file name
  // md5 checksum and properties like password
  explicit FbossFirmware(struct FwAttributes fwAttr)
      : firmwareAttributes_(fwAttr) {}

  // Reads and validates the firmware file checksum
  void load();

  // Get the property values like "msa_password"
  const std::string& getProperty(const std::string& name) const;

  // Provides the image payload pointer to the caller
  folly::io::Cursor getImage() const;

  // Prints the information regarding this image
  void dumpFwInfo();

 protected:
  // Firmware attribute for this firmware
  const struct FwAttributes firmwareAttributes_;

  // File IOBuf for the firmware image
  std::unique_ptr<folly::IOBuf> fileIOBuffer_;
};

} // namespace facebook::fboss
