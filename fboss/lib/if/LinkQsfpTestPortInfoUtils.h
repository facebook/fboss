// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#pragma once

#include <string>

namespace facebook::fboss {
class TcvrState;
class LinkQsfpTestPortInfo;
class LinkQsfpTestPortError;

namespace utility {

// Populates the transceiver-derived fields of a LinkQsfpTestPortInfo (vendor,
// firmware, module media interface, and the per-port media interface for the
// given port) from a transceiver's TcvrState. Shared by the link tests and the
// qsfp hardware tests, which both dump per-port/per-transceiver info to the
// fboss_link_qsfp_test_port_info Scuba table.
void populateTransceiverInfoFields(
    LinkQsfpTestPortInfo& portInfo,
    const TcvrState& tcvrState,
    const std::string& portName);

// Populates the transceiver-derived fields (vendor name, part number) of a
// LinkQsfpTestPortError from a transceiver's TcvrState. Used when reporting a
// module that is present in the chassis but absent from the config, so the row
// identifies which optic is unexpectedly plugged in.
void populateTransceiverErrorFields(
    LinkQsfpTestPortError& portError,
    const TcvrState& tcvrState);

} // namespace utility
} // namespace facebook::fboss
