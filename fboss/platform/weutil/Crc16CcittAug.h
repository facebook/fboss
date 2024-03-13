// (c) Meta Platforms, Inc. and affiliates.

#pragma once

#include <cstddef>
#include <cstdint>

// Per requirements in
// https://docs.google.com/document/d/1KX6q2mHSjFU2-eDt5HGidWSRXm5vFfAaZvdOG890eXY

// Adopted from linux/crc-ccitt.h
// verified with https://crccalc.com/ : CRC-16/AUG-CCITT
namespace facebook::fboss::platform::helpers {

uint16_t crc_ccitt_aug(uint8_t const* buffer, size_t len);

} // namespace facebook::fboss::platform::helpers
