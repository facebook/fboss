/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/packet/SflowStructs.h"

#include <cstdint>
#include <vector>

#include <folly/io/Cursor.h>

using namespace folly;
using namespace folly::io;

namespace facebook::fboss::sflow {

void serializeIP(RWPrivateCursor* cursor, folly::IPAddress ip) {
  // We first push the address type
  if (ip.isV4()) {
    cursor->writeBE<uint32_t>(static_cast<uint32_t>(AddressType::IP_V4));
  } else if (ip.isV6()) {
    cursor->writeBE<uint32_t>(static_cast<uint32_t>(AddressType::IP_V6));
  } else {
    cursor->writeBE<uint32_t>(static_cast<uint32_t>(AddressType::UNKNOWN));
  }
  // then push the address in bytes
  cursor->push(ip.bytes(), ip.byteCount());
}

folly::IPAddress deserializeIP(Cursor& cursor) {
  // Read the address type first
  uint32_t addressType = cursor.readBE<uint32_t>();

  if (addressType == static_cast<uint32_t>(AddressType::IP_V4)) {
    // IPv4 address (4 bytes)
    uint8_t addressBytes[4];
    cursor.pull(addressBytes, 4);
    return folly::IPAddress(
        folly::IPAddressV4::fromBinary(folly::ByteRange(addressBytes, 4)));
  } else if (addressType == static_cast<uint32_t>(AddressType::IP_V6)) {
    // IPv6 address (16 bytes)
    uint8_t addressBytes[16];
    cursor.pull(addressBytes, 16);
    return folly::IPAddress(
        folly::IPAddressV6::fromBinary(folly::ByteRange(addressBytes, 16)));
  } else {
    // Unknown address type
    throw std::runtime_error(
        "Unknown IP address type: " + std::to_string(addressType));
  }
}

uint32_t sizeIP(folly::IPAddress const& ip) {
  return 4 + ip.byteCount();
}

void serializeDataFormat(RWPrivateCursor* cursor, DataFormat fmt) {
  cursor->writeBE<DataFormat>(fmt);
}

void serializeSflowDataSource(RWPrivateCursor* cursor, SflowDataSource src) {
  cursor->writeBE<SflowDataSource>(src);
}

void serializeSflowPort(RWPrivateCursor* cursor, SflowPort sflowPort) {
  cursor->writeBE<SflowPort>(sflowPort);
}

void FlowRecord::serialize(RWPrivateCursor* cursor) const {
  serializeDataFormat(cursor, this->flowFormat);

  // Calculate the size of the flow data
  uint32_t dataSize = std::visit(
      [](const auto& data) -> uint32_t { return data.size(); }, this->flowData);

  // Write the flow data length
  cursor->writeBE<uint32_t>(dataSize);

  // Serialize the flow data based on its type
  std::visit(
      [cursor](const auto& data) { data.serialize(cursor); }, this->flowData);
}

uint32_t FlowRecord::size() const {
  uint32_t dataSize = std::visit(
      [](const auto& data) -> uint32_t { return data.size(); }, this->flowData);

  return 4 /* flowFormat */ + 4 /* flowDataLen */ + dataSize;
}

FlowRecord FlowRecord::deserialize(Cursor& cursor) {
  FlowRecord flowRecord;

  // Read the flow format
  flowRecord.flowFormat = cursor.readBE<DataFormat>();

  // Read the flow data length
  uint32_t flowDataLen = cursor.readBE<uint32_t>();

  // For now, we only support SampledHeader (format = 1)
  // This corresponds to the raw packet header format from sFlow v5 spec
  if (flowRecord.flowFormat == 1) {
    // Create a cursor that limits reading to exactly flowDataLen bytes
    auto limitedBuf = folly::IOBuf::create(flowDataLen);
    cursor.pull(limitedBuf->writableData(), flowDataLen);
    limitedBuf->append(flowDataLen);

    folly::io::Cursor limitedCursor(limitedBuf.get());
    SampledHeader sampledHeader = SampledHeader::deserialize(limitedCursor);
    flowRecord.flowData = std::move(sampledHeader);
  } else {
    throw std::runtime_error(
        "Unsupported flow format: " + std::to_string(flowRecord.flowFormat));
  }

  // Skip XDR padding if needed
  if (flowDataLen % XDR_BASIC_BLOCK_SIZE != 0) {
    uint32_t paddingBytes =
        XDR_BASIC_BLOCK_SIZE - (flowDataLen % XDR_BASIC_BLOCK_SIZE);
    cursor.skip(paddingBytes);
  }

  return flowRecord;
}

void FlowSample::serialize(RWPrivateCursor* cursor) const {
  cursor->writeBE<uint32_t>(this->sequenceNumber);
  serializeSflowDataSource(cursor, this->sourceID);
  cursor->writeBE<uint32_t>(this->samplingRate);
  cursor->writeBE<uint32_t>(this->samplePool);
  cursor->writeBE<uint32_t>(this->drops);
  serializeSflowPort(cursor, this->input);
  serializeSflowPort(cursor, this->output);
  cursor->writeBE<uint32_t>(static_cast<uint32_t>(this->flowRecords.size()));
  for (const FlowRecord& record : this->flowRecords) {
    record.serialize(cursor);
  }
}

uint32_t FlowSample::size() const {
  uint32_t recordsSize = 0;
  for (const auto& record : this->flowRecords) {
    recordsSize += record.size();
  }
  return 4 /* sequenceNumber */ + 4 /* sourceId */ + 4 /* samplingRate */ +
      4 /* samplePool */ + 4 /* drops */ + 4 /* input */ + 4 /* output */ +
      4 /* flowRecordCnt */ + recordsSize;
}

FlowSample FlowSample::deserialize(Cursor& cursor) {
  FlowSample flowSample;

  // Read the basic fields
  flowSample.sequenceNumber = cursor.readBE<uint32_t>();
  flowSample.sourceID = cursor.readBE<SflowDataSource>();
  flowSample.samplingRate = cursor.readBE<uint32_t>();
  flowSample.samplePool = cursor.readBE<uint32_t>();
  flowSample.drops = cursor.readBE<uint32_t>();
  flowSample.input = cursor.readBE<SflowPort>();
  flowSample.output = cursor.readBE<SflowPort>();

  // Read the flow records count
  uint32_t flowRecordsCount = cursor.readBE<uint32_t>();

  // Read each flow record
  flowSample.flowRecords.reserve(flowRecordsCount);
  for (uint32_t i = 0; i < flowRecordsCount; ++i) {
    flowSample.flowRecords.push_back(FlowRecord::deserialize(cursor));
  }

  return flowSample;
}

void SampleRecord::serialize(RWPrivateCursor* cursor) const {
  serializeDataFormat(cursor, this->sampleType);

  // Calculate total data size for all sample data
  uint32_t totalDataSize = 0;
  for (const auto& sampleDatum : this->sampleData) {
    std::visit(
        [&totalDataSize](const auto& data) { totalDataSize += data.size(); },
        sampleDatum);
  }

  cursor->writeBE<uint32_t>(totalDataSize);

  // Serialize each sample data
  for (const auto& sampleDatum : this->sampleData) {
    std::visit(
        [cursor](const auto& data) { data.serialize(cursor); }, sampleDatum);
  }

  // Add XDR padding if needed
  if (totalDataSize % XDR_BASIC_BLOCK_SIZE > 0) {
    int fillCnt = XDR_BASIC_BLOCK_SIZE - totalDataSize % XDR_BASIC_BLOCK_SIZE;
    std::vector<byte> crud(XDR_BASIC_BLOCK_SIZE, 0);
    cursor->push(crud.data(), fillCnt);
  }
}

uint32_t SampleRecord::size() const {
  uint32_t totalDataSize = 0;
  for (const auto& sampleDatum : this->sampleData) {
    std::visit(
        [&totalDataSize](const auto& data) { totalDataSize += data.size(); },
        sampleDatum);
  }

  // Add XDR padding to align to 4-byte boundary (same as serialization does)
  if (totalDataSize % XDR_BASIC_BLOCK_SIZE > 0) {
    totalDataSize +=
        XDR_BASIC_BLOCK_SIZE - (totalDataSize % XDR_BASIC_BLOCK_SIZE);
  }

  return 4 /* sampleType */ + 4 /* sampleDataLen */ + totalDataSize;
}

SampleRecord SampleRecord::deserialize(Cursor& cursor) {
  SampleRecord sampleRecord;

  // Read the sample type
  sampleRecord.sampleType = cursor.readBE<DataFormat>();

  // Read the sample data length
  uint32_t sampleDataLen = cursor.readBE<uint32_t>();

  // Process the sample data if there is any
  if (sampleDataLen == 0) {
    return sampleRecord;
  }
  if (sampleRecord.sampleType == 1) {
    // FlowSample type - deserialize the FlowSample directly
    // The FlowSample::deserialize will read exactly what it needs
    FlowSample flowSample = FlowSample::deserialize(cursor);
    sampleRecord.sampleData.emplace_back(std::move(flowSample));
  } else {
    // Unknown sample type - skip the sample data bytes
    cursor.skip(sampleDataLen);
  }

  // Skip XDR padding if needed to align to 4-byte boundary
  if (sampleDataLen % XDR_BASIC_BLOCK_SIZE > 0) {
    uint32_t paddingBytes =
        XDR_BASIC_BLOCK_SIZE - (sampleDataLen % XDR_BASIC_BLOCK_SIZE);
    cursor.skip(paddingBytes);
  }

  return sampleRecord;
}

void SampleDatagramV5::serialize(RWPrivateCursor* cursor) const {
  serializeIP(cursor, this->agentAddress);
  cursor->writeBE<uint32_t>(this->subAgentID);
  cursor->writeBE<uint32_t>(this->sequenceNumber);
  cursor->writeBE<uint32_t>(this->uptime);
  cursor->writeBE<uint32_t>(static_cast<uint32_t>(this->samples.size()));
  for (const SampleRecord& sample : this->samples) {
    sample.serialize(cursor);
  }
}

uint32_t SampleDatagramV5::size() const {
  uint32_t samplesSize = 0;
  for (const auto& sample : this->samples) {
    samplesSize += sample.size();
  }
  return 4 /* IP address type */ + this->agentAddress.byteCount() +
      4 /* subAgentID */ + 4 /*sequenceNumber */ + 4 /*uptime*/ +
      4 /*samplesCnt */ + samplesSize;
}

SampleDatagramV5 SampleDatagramV5::deserialize(Cursor& cursor) {
  SampleDatagramV5 datagram;

  // Deserialize the agent IP address
  datagram.agentAddress = deserializeIP(cursor);

  // Read the basic fields
  datagram.subAgentID = cursor.readBE<uint32_t>();
  datagram.sequenceNumber = cursor.readBE<uint32_t>();
  datagram.uptime = cursor.readBE<uint32_t>();

  // Read the samples count
  uint32_t samplesCount = cursor.readBE<uint32_t>();

  // Read each sample record
  datagram.samples.reserve(samplesCount);
  for (uint32_t i = 0; i < samplesCount; ++i) {
    datagram.samples.push_back(SampleRecord::deserialize(cursor));
  }

  return datagram;
}

void SampleDatagram::serialize(RWPrivateCursor* cursor) const {
  cursor->writeBE<uint32_t>(SampleDatagram::VERSION5);
  this->datagramV5.serialize(cursor);
}

uint32_t SampleDatagram::size() const {
  return 4 + this->datagramV5.size();
}

SampleDatagram SampleDatagram::deserialize(Cursor& cursor) {
  SampleDatagram datagram;

  // Read and verify the version
  uint32_t version = cursor.readBE<uint32_t>();
  if (version != SampleDatagram::VERSION5) {
    throw std::runtime_error(
        "Unsupported sFlow version: " + std::to_string(version));
  }

  // Deserialize the v5 datagram
  datagram.datagramV5 = SampleDatagramV5::deserialize(cursor);

  return datagram;
}

void SampledHeader::serialize(RWPrivateCursor* cursor) const {
  cursor->writeBE<uint32_t>(static_cast<uint32_t>(this->protocol));
  cursor->writeBE<uint32_t>(this->frameLength);
  cursor->writeBE<uint32_t>(this->stripped);
  cursor->writeBE<uint32_t>(static_cast<uint32_t>(this->header.size()));
  cursor->push(this->header.data(), this->header.size());
  if (this->header.size() % XDR_BASIC_BLOCK_SIZE > 0) {
    int fillCnt =
        XDR_BASIC_BLOCK_SIZE - this->header.size() % XDR_BASIC_BLOCK_SIZE;
    std::vector<byte> crud(XDR_BASIC_BLOCK_SIZE, 0);
    cursor->push(crud.data(), fillCnt);
  }
}

uint32_t SampledHeader::size() const {
  uint32_t headerSize = static_cast<uint32_t>(this->header.size());
  // Add XDR padding to align to 4-byte boundary (same as serialization does)
  if (headerSize % XDR_BASIC_BLOCK_SIZE > 0) {
    headerSize += XDR_BASIC_BLOCK_SIZE - (headerSize % XDR_BASIC_BLOCK_SIZE);
  }
  return 4 /* protocol */ + 4 /* frameLength */ + 4 /* stripped */ +
      4 /* headerLength */ + headerSize;
}

SampledHeader SampledHeader::deserialize(Cursor& cursor) {
  SampledHeader sampledHeader;

  // Read the protocol field
  sampledHeader.protocol =
      static_cast<HeaderProtocol>(cursor.readBE<uint32_t>());

  // Read the frame length
  sampledHeader.frameLength = cursor.readBE<uint32_t>();

  // Read the stripped field
  sampledHeader.stripped = cursor.readBE<uint32_t>();

  // Read the header length
  uint32_t headerLength = cursor.readBE<uint32_t>();

  // Read the header data
  sampledHeader.header.resize(headerLength);
  if (headerLength > 0) {
    cursor.pull(sampledHeader.header.data(), headerLength);
  }

  // Skip XDR padding if needed to align to 4-byte boundary
  if (headerLength % XDR_BASIC_BLOCK_SIZE > 0) {
    uint32_t paddingBytes =
        XDR_BASIC_BLOCK_SIZE - (headerLength % XDR_BASIC_BLOCK_SIZE);
    cursor.skip(paddingBytes);
  }

  return sampledHeader;
}

} // namespace facebook::fboss::sflow
