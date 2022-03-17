// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once

#include <cstdint>
#include <string>

namespace rackmon {

enum class ModbusErrorCode {
  // The function code received in the query
  // is not an allowable action for the slave.
  // If a Poll Program Complete command
  // was issued, this code indicates that no
  // program function preceded it.
  ILLEGAL_FUNCTION = 1,

  // The data address received in the query
  // is not an allowable address for the
  // slave.
  ILLEGAL_DATA_ADDRESS = 2,

  // A value contained in the query data
  // field is not an allowable value for the
  // slave.
  ILLEGAL_DATA_VALUE = 3,

  // An unrecoverable error occurred while
  // the slave was attempting to perform the
  // requested action.
  SLAVE_DEVICE_FAILURE = 4,

  // The slave has accepted the request
  // and is processing it, but a long duration
  // of time will be required to do so. This
  // response is returned to prevent a
  // timeout error from occurring in the
  // master. The master can next issue a
  // Poll Program Complete message to
  // determine if processing is completed.
  ACKNOWLEDGE = 5,

  // The slave is engaged in processing a
  // long–duration program command. The
  // master should retransmit the message
  // later when the slave is free.
  SLAVE_DEVICE_BUSY = 6,

  // The slave cannot perform the program
  // function received in the query. This
  // code is returned for an unsuccessful
  // programming request using function
  // code 13 or 14 decimal. The master
  // should request diagnostic or error
  // information from the slave.
  NEGATIVE_ACKNOWLEDGE = 7,

  //  The slave attempted to read extended
  //  memory, but detected a parity error in
  //  the memory. The master can retry the
  //  request, but service may be required on
  //  the slave device.
  MEMORY_PARITY_ERROR = 8,
  LAST_DEFINED_ERROR = 8,
  UNDEFINED_ERROR = 255,
};

struct ModbusError : public std::runtime_error {
  uint8_t errorData;
  ModbusErrorCode errorCode;
  ModbusError(uint8_t error)
      : std::runtime_error("Modbus Error: " + std::to_string(int(error))),
        errorData(error) {
    if (error <= static_cast<uint8_t>(ModbusErrorCode::LAST_DEFINED_ERROR)) {
      errorCode = static_cast<ModbusErrorCode>(error);
    } else {
      errorCode = ModbusErrorCode::UNDEFINED_ERROR;
    }
  }
};

} // namespace rackmon
