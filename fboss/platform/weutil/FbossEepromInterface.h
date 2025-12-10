// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "fboss/platform/weutil/if/gen-cpp2/eeprom_contents_types.h"

namespace facebook::fboss::platform {

class FbossEepromInterface {
 public:
  enum entryType {
    FIELD_INVALID,
    FIELD_BE_UINT,
    FIELD_BE_HEX,
    FIELD_STRING,
    FIELD_MAC,
    FIELD_DATE
  };

  struct EepromFieldEntry {
    std::string fieldName;
    entryType fieldType;
    std::optional<int> length{std::nullopt};
    std::string value{};
  };

  FbossEepromInterface(const std::string& eepromPath, const uint16_t offset);

  // TODO: Get rid of getContents() in the future.
  std::vector<std::pair<std::string, std::string>> getContents() const;

  int getVersion() const;
  std::string getProductName() const;
  std::string getProductPartNumber() const;
  std::string getProductionState() const;
  std::string getProductionSubState() const;
  std::string getVariantVersion() const;
  std::string getProductSerialNumber() const;

  EepromContents getEepromContents() const;

 private:
  FbossEepromInterface() = default;
  void parseEepromBlobTLV(const std::vector<uint8_t>& buffer);

  std::map<int, EepromFieldEntry> fieldMap_{};
  int version_{0};
};

} // namespace facebook::fboss::platform
