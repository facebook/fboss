/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/packet/bcm/XgsPsampMod.h"

#include <fmt/core.h>

#include "fboss/agent/packet/HdrParseError.h"

using namespace folly::io;

namespace facebook::fboss::psamp {

void XgsPsampTemplateHeader::serialize(RWPrivateCursor* cursor) const {
  cursor->writeBE<uint16_t>(templateId);
  cursor->writeBE<uint16_t>(psampLength);
}

uint32_t XgsPsampTemplateHeader::size() const {
  return 4;
}

XgsPsampTemplateHeader XgsPsampTemplateHeader::deserialize(Cursor& cursor) {
  if (cursor.totalLength() < 4) {
    throw HdrParseError(
        "PSAMP template header too small: need 4 bytes, have " +
        std::to_string(cursor.totalLength()));
  }

  XgsPsampTemplateHeader hdr;
  hdr.templateId = cursor.readBE<uint16_t>();
  if (hdr.templateId != XGS_PSAMP_TEMPLATE_ID) {
    throw HdrParseError(
        "Unexpected PSAMP template ID: expected 0x" +
        fmt::format("{:04X}", XGS_PSAMP_TEMPLATE_ID) + ", got 0x" +
        fmt::format("{:04X}", hdr.templateId));
  }
  hdr.psampLength = cursor.readBE<uint16_t>();
  return hdr;
}

void XgsPsampData::serialize(RWPrivateCursor* cursor) const {
  cursor->writeBE<uint64_t>(observationTimeNs);
  cursor->writeBE<uint32_t>(switchId);
  cursor->writeBE<uint16_t>(egressModPortId);
  cursor->writeBE<uint16_t>(ingressPort);
  cursor->write<uint8_t>(dropReasonIngress);
  cursor->write<uint8_t>(dropReasonMmu);
  cursor->writeBE<uint16_t>(userMetaField);
  cursor->write<uint8_t>(cosColorProb);
  cursor->write<uint8_t>(varLenIndicator);
  cursor->writeBE<uint16_t>(packetSampledLength);
  cursor->push(sampledPacketData.data(), sampledPacketData.size());
}

uint32_t XgsPsampData::size() const {
  return static_cast<uint32_t>(24 + sampledPacketData.size());
}

XgsPsampData XgsPsampData::deserialize(Cursor& cursor) {
  if (cursor.totalLength() < 24) {
    throw HdrParseError(
        "PSAMP data too small: need 24 bytes, have " +
        std::to_string(cursor.totalLength()));
  }

  XgsPsampData data;
  data.observationTimeNs = cursor.readBE<uint64_t>();
  data.switchId = cursor.readBE<uint32_t>();
  data.egressModPortId = cursor.readBE<uint16_t>();
  data.ingressPort = cursor.readBE<uint16_t>();
  data.dropReasonIngress = cursor.read<uint8_t>();
  data.dropReasonMmu = cursor.read<uint8_t>();
  data.userMetaField = cursor.readBE<uint16_t>();
  data.cosColorProb = cursor.read<uint8_t>();
  data.varLenIndicator = cursor.read<uint8_t>();
  if (data.varLenIndicator != XGS_PSAMP_VAR_LEN_INDICATOR) {
    throw HdrParseError(
        "Invalid PSAMP variable length indicator: expected 0xFF, got 0x" +
        fmt::format("{:02X}", data.varLenIndicator));
  }
  data.packetSampledLength = cursor.readBE<uint16_t>();
  if (cursor.totalLength() < data.packetSampledLength) {
    throw HdrParseError(
        "PSAMP sampled packet data too small: need " +
        std::to_string(data.packetSampledLength) + " bytes, have " +
        std::to_string(cursor.totalLength()));
  }
  data.sampledPacketData.resize(data.packetSampledLength);
  cursor.pull(data.sampledPacketData.data(), data.packetSampledLength);
  return data;
}

void XgsPsampModPacket::serialize(RWPrivateCursor* cursor) const {
  ipfixHeader.serialize(cursor);
  templateHeader.serialize(cursor);
  data.serialize(cursor);
}

uint32_t XgsPsampModPacket::size() const {
  return ipfixHeader.size() + templateHeader.size() + data.size();
}

XgsPsampModPacket XgsPsampModPacket::deserialize(Cursor& cursor) {
  XgsPsampModPacket pkt;
  pkt.ipfixHeader = IpfixHeader::deserialize(cursor);
  pkt.templateHeader = XgsPsampTemplateHeader::deserialize(cursor);
  pkt.data = XgsPsampData::deserialize(cursor);
  if (pkt.ipfixHeader.length != pkt.size()) {
    throw HdrParseError(
        "IPFIX length mismatch: header says " +
        std::to_string(pkt.ipfixHeader.length) + " but actual size is " +
        std::to_string(pkt.size()));
  }
  return pkt;
}

} // namespace facebook::fboss::psamp
