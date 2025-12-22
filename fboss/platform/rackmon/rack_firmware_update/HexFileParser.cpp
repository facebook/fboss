/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/rackmon/rack_firmware_update/HexFileParser.h"
#include "fboss/platform/rackmon/rack_firmware_update/UpdaterExceptions.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace facebook::fboss::platform::rackmon {

namespace {

// Intel HEX record types
enum RecordType {
  DATA = 0,
  END_OF_FILE = 1,
  EXTENDED_SEGMENT_ADDRESS = 2,
  START_SEGMENT_ADDRESS = 3,
  EXTENDED_LINEAR_ADDRESS = 4,
  START_LINEAR_ADDRESS = 5,
};

uint16_t makeWord(uint8_t msb, uint8_t lsb) {
  return (static_cast<uint16_t>(msb) << 8) | lsb;
}

uint32_t makeDword(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4) {
  return (static_cast<uint32_t>(b1) << 24) | (static_cast<uint32_t>(b2) << 16) |
      (static_cast<uint32_t>(b3) << 8) | b4;
}

uint8_t hexCharToByte(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  throw FileException(std::string("Invalid hex character: ") + c);
}

uint8_t hexPairToByte(const std::string& str, size_t offset) {
  return (hexCharToByte(str[offset]) << 4) | hexCharToByte(str[offset + 1]);
}

} // namespace

size_t HexFile::totalSize() const {
  size_t total = 0;
  for (const auto& seg : segments_) {
    total += seg.size();
  }
  return total;
}

HexFile HexFile::load(const std::string& filename) {
  std::ifstream file(filename);
  if (!file) {
    throw FileException("Failed to open hex file: " + filename);
  }

  HexFile hexFile;
  std::string line;
  int lineNo = 0;
  bool endOfFile = false;
  uint32_t extendedLinearAddress = 0;
  uint32_t extendedSegmentAddress = 0;

  while (std::getline(file, line)) {
    lineNo++;

    // Skip empty lines
    if (line.empty() || line[0] != ':') {
      continue;
    }

    if (endOfFile) {
      throw FileException(
          "Record found after end of file at line " + std::to_string(lineNo));
    }

    // Parse hex bytes
    std::vector<uint8_t> bytes;
    try {
      for (size_t i = 1; i + 1 < line.length(); i += 2) {
        bytes.push_back(hexPairToByte(line, i));
      }
    } catch (const std::exception& e) {
      throw FileException(
          "Parse error at line " + std::to_string(lineNo) + ": " + e.what());
    }

    if (bytes.size() < 5) {
      throw FileException("Record too short at line " + std::to_string(lineNo));
    }

    // Extract fields
    uint8_t byteCount = bytes[0];
    uint16_t address = makeWord(bytes[1], bytes[2]);
    uint8_t recordType = bytes[3];
    uint8_t checksum = bytes[bytes.size() - 1];

    // Verify checksum
    uint8_t computedChecksum = 0;
    for (size_t i = 0; i < bytes.size() - 1; i++) {
      computedChecksum += bytes[i];
    }
    computedChecksum = (~computedChecksum + 1) & 0xFF;

    if (computedChecksum != checksum) {
      throw FileException(
          "Checksum mismatch at line " + std::to_string(lineNo));
    }

    // Extract data bytes
    std::vector<uint8_t> data(bytes.begin() + 4, bytes.begin() + 4 + byteCount);

    if (data.size() != byteCount) {
      throw FileException(
          "Data size mismatch at line " + std::to_string(lineNo));
    }

    // Process based on record type
    switch (recordType) {
      case DATA: {
        uint32_t currentAddress =
            (address + extendedLinearAddress + extendedSegmentAddress) &
            0xFFFFFFFF;

        // Try to append to existing segment if contiguous
        bool appended = false;
        for (auto& seg : hexFile.segments_) {
          if (seg.endAddress() == currentAddress) {
            seg.data.insert(seg.data.end(), data.begin(), data.end());
            appended = true;
            break;
          }
        }

        // Create new segment if not appended
        if (!appended) {
          HexSegment seg;
          seg.startAddress = currentAddress;
          seg.data = data;
          hexFile.segments_.push_back(seg);
        }
        break;
      }

      case END_OF_FILE:
        endOfFile = true;
        break;

      case EXTENDED_SEGMENT_ADDRESS:
        if (byteCount != 2) {
          throw FileException(
              "Invalid extended segment address record at line " +
              std::to_string(lineNo));
        }
        extendedSegmentAddress = makeWord(data[0], data[1]) << 4;
        break;

      case START_SEGMENT_ADDRESS:
        if (byteCount != 4) {
          throw FileException(
              "Invalid start segment address record at line " +
              std::to_string(lineNo));
        }
        hexFile.cs_ = makeWord(data[0], data[1]);
        hexFile.ip_ = makeWord(data[2], data[3]);
        hexFile.hasCSIP_ = true;
        break;

      case EXTENDED_LINEAR_ADDRESS:
        if (byteCount != 2) {
          throw FileException(
              "Invalid extended linear address record at line " +
              std::to_string(lineNo));
        }
        extendedLinearAddress = makeWord(data[0], data[1]) << 16;
        break;

      case START_LINEAR_ADDRESS:
        if (byteCount != 4) {
          throw FileException(
              "Invalid start linear address record at line " +
              std::to_string(lineNo));
        }
        hexFile.eip_ = makeDword(data[0], data[1], data[2], data[3]);
        hexFile.hasEIP_ = true;
        break;

      default:
        throw FileException(
            "Unknown record type at line " + std::to_string(lineNo) + ": " +
            std::to_string(recordType));
    }
  }

  return hexFile;
}

} // namespace facebook::fboss::platform::rackmon
