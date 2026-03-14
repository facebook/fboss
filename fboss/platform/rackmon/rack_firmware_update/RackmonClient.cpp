/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>

#include "fboss/platform/rackmon/if/gen-cpp2/rackmonsvc_clients.h"
#include "fboss/platform/rackmon/rack_firmware_update/RackmonClient.h"
#include "fboss/platform/rackmon/rack_firmware_update/UpdaterExceptions.h"

namespace facebook::fboss::platform::rackmon {

using namespace rackmonsvc;

RackmonClient::RackmonClient(const std::string& host, uint16_t port)
    : host_(host), port_(port) {
  ensureConnected();
}

RackmonClient::~RackmonClient() {
  // Cleanup is automatic via unique_ptr and shared_ptr
}

void RackmonClient::ensureConnected() {
  // Check if we need to create or recreate the connection
  if (!client_ || !channel_ || !evb_) {
    evb_ = std::make_unique<folly::EventBase>();
    auto socket = folly::AsyncSocket::newSocket(evb_.get(), host_, port_);
    channel_ =
        apache::thrift::RocketClientChannel::newChannel(std::move(socket));
    client_ =
        std::make_unique<apache::thrift::Client<::rackmonsvc::RackmonCtrl>>(
            channel_);
  }
}

void RackmonClient::checkStatus(::rackmonsvc::RackmonStatusCode status) {
  switch (status) {
    case ::rackmonsvc::RackmonStatusCode::SUCCESS:
      return;
    case ::rackmonsvc::RackmonStatusCode::ERR_BAD_CRC:
      throw ModbusCRCException();
    case ::rackmonsvc::RackmonStatusCode::ERR_TIMEOUT:
      throw ModbusTimeoutException();
    case ::rackmonsvc::RackmonStatusCode::ERR_INVALID_ARGS:
      throw UpdaterException("Invalid arguments");
    case ::rackmonsvc::RackmonStatusCode::ERR_IO_FAILURE:
      throw UpdaterException("Modbus I/O failure");
    default:
      throw UpdaterException(
          "Unknown error: " + std::to_string(static_cast<int>(status)));
  }
}

std::vector<uint16_t> RackmonClient::readHoldingRegisters(
    uint16_t devAddress,
    uint16_t regAddress,
    uint16_t numRegisters,
    uint32_t timeout) {
  try {
    ensureConnected();

    ReadWordRegistersRequest req;
    req.devAddress() = devAddress;
    req.regAddress() = regAddress;
    req.numRegisters() = numRegisters;
    if (timeout > 0) {
      req.timeout() = timeout;
    }

    ReadWordRegistersResponse resp;
    client_->sync_readHoldingRegisters(resp, req);

    checkStatus(*resp.status());

    std::vector<uint16_t> result;
    for (auto val : *resp.regValues()) {
      result.push_back(val);
    }
    return result;
  } catch (const apache::thrift::TApplicationException& e) {
    throw UpdaterException("Thrift error: " + std::string(e.what()));
  }
}

void RackmonClient::writeSingleRegister(
    uint16_t devAddress,
    uint16_t regAddress,
    uint16_t value,
    uint32_t timeout) {
  try {
    ensureConnected();

    WriteSingleRegisterRequest req;
    req.devAddress() = devAddress;
    req.regAddress() = regAddress;
    req.regValue() = value;
    if (timeout > 0) {
      req.timeout() = timeout;
    }

    auto status = client_->sync_writeSingleRegister(req);
    checkStatus(status);
  } catch (const apache::thrift::TApplicationException& e) {
    throw UpdaterException("Thrift error: " + std::string(e.what()));
  }
}

void RackmonClient::presetMultipleRegisters(
    uint16_t devAddress,
    uint16_t regAddress,
    const std::vector<uint16_t>& values,
    uint32_t timeout) {
  try {
    ensureConnected();

    PresetMultipleRegistersRequest req;
    req.devAddress() = devAddress;
    req.regAddress() = regAddress;
    // Convert uint16_t to int32_t for thrift
    std::vector<int32_t> int32Values;
    for (auto val : values) {
      int32Values.push_back(static_cast<int32_t>(val));
    }
    req.regValue() = int32Values;
    if (timeout > 0) {
      req.timeout() = timeout;
    }

    auto status = client_->sync_presetMultipleRegisters(req);
    checkStatus(status);
  } catch (const apache::thrift::TApplicationException& e) {
    throw UpdaterException("Thrift error: " + std::string(e.what()));
  }
}

std::vector<uint8_t> RackmonClient::sendRawCommand(
    const std::vector<uint8_t>& cmd,
    uint32_t expectedLength,
    uint32_t timeout,
    uint16_t uniqueDevAddr) {
  try {
    ensureConnected();

    RawCommandRequest req;
    // Convert unsigned bytes (0-255) to signed bytes (-128 to 127) for thrift
    std::vector<int8_t> signedCmd;
    for (uint8_t byte : cmd) {
      signedCmd.push_back(byte < 128 ? byte : byte - 256);
    }
    req.cmd() = signedCmd;
    req.responseLength() = expectedLength;
    if (timeout > 0) {
      req.timeout() = timeout;
    }
    if (uniqueDevAddr > 0) {
      req.uniqueDevAddress() = uniqueDevAddr;
    }

    RawCommandResponse resp;
    client_->sync_sendRawCommand(resp, req);

    checkStatus(*resp.status());

    // Convert signed bytes back to unsigned
    std::vector<uint8_t> result;
    for (int8_t byte : *resp.data()) {
      result.push_back(byte >= 0 ? byte : byte + 256);
    }
    return result;
  } catch (const apache::thrift::TApplicationException& e) {
    throw UpdaterException("Thrift error: " + std::string(e.what()));
  }
}

void RackmonClient::pauseMonitoring() {
  try {
    ensureConnected();

    auto status =
        client_->sync_controlRackmond(RackmonControlRequest::PAUSE_RACKMOND);
    checkStatus(status);
  } catch (const apache::thrift::TApplicationException& e) {
    throw UpdaterException("Thrift error: " + std::string(e.what()));
  }
}

void RackmonClient::resumeMonitoring() {
  try {
    ensureConnected();

    auto status =
        client_->sync_controlRackmond(RackmonControlRequest::RESUME_RACKMOND);
    checkStatus(status);
  } catch (const apache::thrift::TApplicationException& e) {
    throw UpdaterException("Thrift error: " + std::string(e.what()));
  }
}

} // namespace facebook::fboss::platform::rackmon
