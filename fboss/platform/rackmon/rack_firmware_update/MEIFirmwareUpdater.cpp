/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/rackmon/rack_firmware_update/MEIFirmwareUpdater.h"
#include "fboss/platform/rackmon/rack_firmware_update/UpdaterExceptions.h"
#include "fboss/platform/rackmon/rack_firmware_update/UpdaterUtils.h"

#include <fmt/format.h>
#include <chrono>
#include <iostream>
#include <sstream>

namespace facebook::fboss::platform::rackmon {

// StatusRegister implementation
StatusRegister::StatusRegister(const std::array<uint8_t, 4>& bytes) {
  // Big-endian: bytes[0] is MSB
  value_ = (static_cast<uint32_t>(bytes[0]) << 24) |
      (static_cast<uint32_t>(bytes[1]) << 16) |
      (static_cast<uint32_t>(bytes[2]) << 8) | bytes[3];
}

std::string StatusRegister::toString() const {
  return fmt::format("0x{:08x}", value_);
}

// MEIFirmwareUpdater implementation
MEIFirmwareUpdater::MEIFirmwareUpdater(
    std::shared_ptr<RackmonClient> client,
    uint16_t deviceAddr,
    uint64_t securityKey)
    : FirmwareUpdater(std::move(client), deviceAddr),
      securityKey_(securityKey) {}

std::vector<uint8_t> MEIFirmwareUpdater::buildMEICommand(
    const std::vector<uint8_t>& cmd) const {
  std::vector<uint8_t> fullCmd;
  fullCmd.push_back(static_cast<uint8_t>(deviceAddr_ & 0xFF));
  fullCmd.insert(fullCmd.end(), cmd.begin(), cmd.end());
  return fullCmd;
}

StatusRegister MEIFirmwareUpdater::getStatusRegister() {
  // MEI command: 0x2B 0x64 0x22 0x00 0x00
  std::vector<uint8_t> cmd = {0x2B, 0x64, 0x22, 0x00, 0x00};
  auto fullCmd = buildMEICommand(cmd);

  // Expected response: addr + 0x2B 0x71 0x62 0x00 0x00 + 4 bytes status
  auto resp = client_->sendRawCommand(fullCmd, 12);

  // Check response format
  std::vector<uint8_t> expectedPrefix = {
      static_cast<uint8_t>(deviceAddr_ & 0xFF), 0x2B, 0x71, 0x62, 0x00, 0x00};
  if (resp.size() != 10 ||
      !std::equal(expectedPrefix.begin(), expectedPrefix.end(), resp.begin())) {
    throw BadMEIResponseException("Bad status response: " + bytesToHex(resp));
  }

  // Extract status bytes (big-endian, 4 bytes)
  std::array<uint8_t, 4> statusBytes = {resp[6], resp[7], resp[8], resp[9]};
  return StatusRegister(statusBytes);
}

StatusRegister MEIFirmwareUpdater::waitStatus(
    std::optional<StatusRegister::BitField> bitSet,
    std::optional<StatusRegister::BitField> bitCleared,
    double timeout,
    double delay) {
  auto start = std::chrono::steady_clock::now();
  auto timeoutMs = std::chrono::milliseconds(static_cast<int>(timeout * 1000));
  auto delayMs = std::chrono::milliseconds(static_cast<int>(delay * 1000));

  while (true) {
    auto status = getStatusRegister();

    // Check if condition is met
    if (bitSet.has_value() && status[bitSet.value()]) {
      return status;
    }
    if (bitCleared.has_value() && !status[bitCleared.value()]) {
      return status;
    }

    // Check timeout
    auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed >= timeoutMs) {
      throw UpdaterException(
          fmt::format(
              "Timeout waiting for status. Current status: {}",
              status.toString()));
    }

    std::this_thread::sleep_for(delayMs);
  }
}

std::array<uint8_t, 4> MEIFirmwareUpdater::getChallenge() {
  std::cout << "Send get seed" << std::endl;

  // MEI command: 0x2B 0x64 0x27 0x00 0x00
  std::vector<uint8_t> cmd = {0x2B, 0x64, 0x27, 0x00, 0x00};
  auto fullCmd = buildMEICommand(cmd);

  // Expected response: addr + 0x2B 0x71 0x67 0x00 0x00 + 4 bytes challenge
  auto resp = client_->sendRawCommand(fullCmd, 12);

  std::vector<uint8_t> expectedPrefix = {
      static_cast<uint8_t>(deviceAddr_ & 0xFF), 0x2B, 0x71, 0x67, 0x00, 0x00};
  if (resp.size() != 10 ||
      !std::equal(expectedPrefix.begin(), expectedPrefix.end(), resp.begin())) {
    throw BadMEIResponseException(
        "Bad challenge response: " + bytesToHex(resp));
  }

  std::array<uint8_t, 4> challenge = {resp[6], resp[7], resp[8], resp[9]};
  std::cout << "Got seed: "
            << bytesToHex(
                   std::vector<uint8_t>(challenge.begin(), challenge.end()))
            << std::endl;
  return challenge;
}

void MEIFirmwareUpdater::sendKey(const std::array<uint8_t, 4>& key) {
  std::cout << "Send key" << std::endl;

  // MEI command: 0x2B 0x64 0x27 0x00 0x01 + 4 bytes key
  std::vector<uint8_t> cmd = {0x2B, 0x64, 0x27, 0x00, 0x01};
  cmd.insert(cmd.end(), key.begin(), key.end());
  auto fullCmd = buildMEICommand(cmd);

  // Expected response: addr + 0x2B 0x71 0x67 0x00 0x01 0xFF 0xFF 0xFF 0xFF
  auto resp = client_->sendRawCommand(fullCmd, 12);

  std::vector<uint8_t> expectedResp = {
      static_cast<uint8_t>(deviceAddr_ & 0xFF),
      0x2B,
      0x71,
      0x67,
      0x00,
      0x01,
      0xFF,
      0xFF,
      0xFF,
      0xFF};
  if (resp != expectedResp) {
    throw BadMEIResponseException("Bad key response: " + bytesToHex(resp));
  }

  std::cout << "Send key successful." << std::endl;
}

std::array<uint8_t, 4> MEIFirmwareUpdater::calculateKey(
    const std::array<uint8_t, 4>& challenge) {
  // Delta security key calculation algorithm
  uint32_t lower = securityKey_ & 0xFFFFFFFF;
  uint32_t upper = (securityKey_ >> 32) & 0xFFFFFFFF;

  // Convert challenge to uint32 (big-endian)
  uint32_t seed = (static_cast<uint32_t>(challenge[0]) << 24) |
      (static_cast<uint32_t>(challenge[1]) << 16) |
      (static_cast<uint32_t>(challenge[2]) << 8) | challenge[3];

  // Algorithm: XOR and shift operations
  for (int i = 0; i < 32; i++) {
    if ((seed & 1) != 0) {
      seed = seed ^ lower;
    }
    seed = (seed >> 1) & 0x7FFFFFFF;
  }
  seed = seed ^ upper;

  // Convert back to bytes (big-endian)
  std::array<uint8_t, 4> result = {
      static_cast<uint8_t>((seed >> 24) & 0xFF),
      static_cast<uint8_t>((seed >> 16) & 0xFF),
      static_cast<uint8_t>((seed >> 8) & 0xFF),
      static_cast<uint8_t>(seed & 0xFF)};
  return result;
}

void MEIFirmwareUpdater::keyHandshake() {
  auto challenge = getChallenge();
  auto key = calculateKey(challenge);
  sendKey(key);
}

void MEIFirmwareUpdater::eraseFlash() {
  std::cout << "Erasing flash... " << std::flush;

  // MEI command: 0x2B 0x64 0x31 0x00 0x00 0xFF 0xFF 0xFF 0xFF
  std::vector<uint8_t> cmd = {
      0x2B, 0x64, 0x31, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
  auto fullCmd = buildMEICommand(cmd);

  // Expected response: addr + 0x2B 0x71 0x71 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
  auto resp = client_->sendRawCommand(fullCmd, 12);

  std::vector<uint8_t> expectedResp = {
      static_cast<uint8_t>(deviceAddr_ & 0xFF),
      0x2B,
      0x71,
      0x71,
      0xFF,
      0xFF,
      0xFF,
      0xFF,
      0xFF,
      0xFF};
  if (resp != expectedResp) {
    throw BadMEIResponseException("Bad erase response: " + bytesToHex(resp));
  }

  sleep(1500);
  auto status = getStatusRegister();
  if (status[StatusRegister::ERASE_DONE]) {
    std::cout << "Erase successful" << std::endl;
  } else {
    throw UpdaterException("Erase failed. Status: " + status.toString());
  }
}

void MEIFirmwareUpdater::setWriteAddress(uint32_t address) {
  // MEI command: 0x2B 0x64 0x34 0x00 0x00 + 4 bytes address (big-endian)
  std::vector<uint8_t> cmd = {0x2B, 0x64, 0x34, 0x00, 0x00};
  cmd.push_back((address >> 24) & 0xFF);
  cmd.push_back((address >> 16) & 0xFF);
  cmd.push_back((address >> 8) & 0xFF);
  cmd.push_back(address & 0xFF);
  auto fullCmd = buildMEICommand(cmd);

  // Expected response: addr + 0x2B 0x71 0x74 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
  auto resp = client_->sendRawCommand(fullCmd, 12);

  std::vector<uint8_t> expectedResp = {
      static_cast<uint8_t>(deviceAddr_ & 0xFF),
      0x2B,
      0x71,
      0x74,
      0xFF,
      0xFF,
      0xFF,
      0xFF,
      0xFF,
      0xFF};
  if (resp != expectedResp) {
    throw BadMEIResponseException(
        "Bad set write addr response: " + bytesToHex(resp));
  }

  waitStatus(StatusRegister::ADD_ACCEPTED, std::nullopt);
}

void MEIFirmwareUpdater::writeData(const std::vector<uint8_t>& data) {
  if (data.size() != CHUNK_SIZE) {
    throw UpdaterException(
        fmt::format(
            "Invalid data size: {} (expected {})", data.size(), CHUNK_SIZE));
  }

  // MEI command: 0x2B 0x65 0x36 + 128 bytes data
  std::vector<uint8_t> cmd = {0x2B, 0x65, 0x36};
  cmd.insert(cmd.end(), data.begin(), data.end());
  auto fullCmd = buildMEICommand(cmd);

  // Expected response: addr + 0x2B 0x73 0x76 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
  auto resp = client_->sendRawCommand(fullCmd, 12);

  std::vector<uint8_t> expectedResp = {
      static_cast<uint8_t>(deviceAddr_ & 0xFF),
      0x2B,
      0x73,
      0x76,
      0xFF,
      0xFF,
      0xFF,
      0xFF,
      0xFF,
      0xFF};
  if (resp != expectedResp) {
    throw BadMEIResponseException(
        "Bad write data response: " + bytesToHex(resp));
  }

  // Give PSU time to process
  sleep(20);

  // Wait till SEND_DATA_BUSY is cleared
  auto status =
      waitStatus(std::nullopt, StatusRegister::SEND_DATA_BUSY, 5.0, 0.05);

  if (status[StatusRegister::SEND_DATA_BUSY]) {
    throw UpdaterException(
        "Write data busy after 5s. Status: " + status.toString());
  }

  // Check if SEND_DATA_RDY is set, if not wait for it
  if (!status[StatusRegister::SEND_DATA_RDY]) {
    status = waitStatus(StatusRegister::SEND_DATA_RDY, std::nullopt, 5.0, 0.05);
    if (!status[StatusRegister::SEND_DATA_RDY]) {
      throw UpdaterException("Write data failed. Status: " + status.toString());
    }
  }
}

void MEIFirmwareUpdater::verifyFlash() {
  std::cout << "Verifying program..." << std::endl;

  // MEI command: 0x2B 0x64 0x31 0x00 0x01
  std::vector<uint8_t> cmd = {0x2B, 0x64, 0x31, 0x00, 0x01};
  auto fullCmd = buildMEICommand(cmd);

  // Expected response: addr + 0x2B 0x71 0x71 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
  auto resp = client_->sendRawCommand(fullCmd, 12);

  std::vector<uint8_t> expectedResp = {
      static_cast<uint8_t>(deviceAddr_ & 0xFF),
      0x2B,
      0x71,
      0x71,
      0xFF,
      0xFF,
      0xFF,
      0xFF,
      0xFF,
      0xFF};
  if (resp != expectedResp) {
    throw BadMEIResponseException("Bad verify response: " + bytesToHex(resp));
  }

  sleep(100);

  // Wait till VERIFY_CRC_BUSY is cleared
  auto status = waitStatus(std::nullopt, StatusRegister::VERIFY_CRC_BUSY);

  if (!status[StatusRegister::CRC_VERIFIED]) {
    throw VerificationException(
        "CRC verification failed. Status: " + status.toString());
  }

  std::cout << "Verify of flash successful!" << std::endl;
}

void MEIFirmwareUpdater::activate() {
  std::cout << "Activating Image..." << std::endl;

  // MEI command: 0x2B 0x64 0x2E 0x00 0x00
  std::vector<uint8_t> cmd = {0x2B, 0x64, 0x2E, 0x00, 0x00};
  auto fullCmd = buildMEICommand(cmd);

  // Expected response: addr + 0x2B 0x71 0x6E 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
  auto resp = client_->sendRawCommand(fullCmd, 12);

  std::vector<uint8_t> expectedResp = {
      static_cast<uint8_t>(deviceAddr_ & 0xFF),
      0x2B,
      0x71,
      0x6E,
      0xFF,
      0xFF,
      0xFF,
      0xFF,
      0xFF,
      0xFF};
  if (resp != expectedResp) {
    throw BadMEIResponseException("Bad activate response: " + bytesToHex(resp));
  }

  std::cout << "Activate successful!" << std::endl;
}

void MEIFirmwareUpdater::sendImage(const HexFile& hexFile) {
  size_t totalChunks = 0;
  for (const auto& seg : hexFile.getSegments()) {
    totalChunks += (seg.size() + CHUNK_SIZE - 1) / CHUNK_SIZE;
  }

  size_t sentChunks = 0;

  for (const auto& seg : hexFile.getSegments()) {
    if (seg.size() == 0) {
      std::cout << "Ignoring empty segment at 0x" << std::hex
                << seg.startAddress << std::dec << std::endl;
      continue;
    }

    std::cout << fmt::format(
                     "Sending <{} byte segment @ 0x{:08x}>",
                     seg.size(),
                     seg.startAddress)
              << std::endl;

    setWriteAddress(seg.startAddress);

    for (size_t i = 0; i < seg.size(); i += CHUNK_SIZE) {
      std::vector<uint8_t> chunk;

      // Get chunk data
      size_t remaining = seg.size() - i;
      if (remaining >= CHUNK_SIZE) {
        chunk.assign(seg.data.begin() + i, seg.data.begin() + i + CHUNK_SIZE);
      } else {
        // Pad with 0xFF
        chunk.assign(seg.data.begin() + i, seg.data.end());
        chunk.resize(CHUNK_SIZE, 0xFF);
      }

      sentChunks++;
      printProgress(
          sentChunks * 100.0 / totalChunks,
          fmt::format("Sending chunk {} of {}...", sentChunks, totalChunks));

      writeData(chunk);
    }
  }

  printProgress(
      100.0,
      fmt::format("Sending chunk {} of {}...", totalChunks, totalChunks));
  std::cout << std::endl;
}

void MEIFirmwareUpdater::updateFirmware(const std::string& firmwareFile) {
  std::cout << "Parsing Firmware" << std::endl;
  auto hexFile = HexFile::load(firmwareFile);

  keyHandshake();
  eraseFlash();
  sendImage(hexFile);
  verifyFlash();
  activate();
}

std::string MEIFirmwareUpdater::readVersion() {
  try {
    // Read firmware version from register 0x0030 (4 registers)
    auto values = client_->readHoldingRegisters(deviceAddr_, 0x0030, 4, 2000);

    // Convert register values to ASCII string
    std::string version;
    for (auto val : values) {
      version += static_cast<char>((val >> 8) & 0xFF);
      version += static_cast<char>(val & 0xFF);
    }

    // Remove null terminators
    version.erase(
        std::find(version.begin(), version.end(), '\0'), version.end());
    return version;
  } catch (const std::exception& e) {
    return "Could not read version: " + std::string(e.what());
  }
}

} // namespace facebook::fboss::platform::rackmon
