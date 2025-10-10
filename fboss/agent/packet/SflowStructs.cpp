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
  cursor->writeBE<uint32_t>(static_cast<uint32_t>(AddressType::IP_V6));
  // then push the address in bytes
  cursor->push(ip.bytes(), ip.byteCount());
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
  // serialize XDR opaque sFlow flow_data
  cursor->writeBE<uint32_t>(static_cast<uint32_t>(this->flowData.size()));
  cursor->push(this->flowData.data(), this->flowData.size());
  if (this->flowData.size() % XDR_BASIC_BLOCK_SIZE != 0) {
    int fillCnt =
        XDR_BASIC_BLOCK_SIZE - this->flowData.size() % XDR_BASIC_BLOCK_SIZE;
    std::vector<byte> crud(XDR_BASIC_BLOCK_SIZE, 0);
    cursor->push(crud.data(), fillCnt);
  }
}

uint32_t FlowRecord::size() const {
  uint32_t dataSize = this->flowData.size();
  // Add XDR padding to align to 4-byte boundary (same as serialization does)
  if (dataSize % XDR_BASIC_BLOCK_SIZE != 0) {
    dataSize += XDR_BASIC_BLOCK_SIZE - (dataSize % XDR_BASIC_BLOCK_SIZE);
  }
  return 4 /* flowFormat */ + 4 /* flowDataLen */ + dataSize;
}

FlowRecord FlowRecord::deserialize(Cursor& cursor) {
  FlowRecord flowRecord;

  // Read the flow format
  flowRecord.flowFormat = cursor.readBE<DataFormat>();

  // Read the flow data length
  uint32_t flowDataLen = cursor.readBE<uint32_t>();

  // Read the flow data
  flowRecord.flowData.resize(flowDataLen);
  cursor.pull(flowRecord.flowData.data(), flowDataLen);

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

void SampleDatagram::serialize(RWPrivateCursor* cursor) const {
  cursor->writeBE<uint32_t>(SampleDatagram::VERSION5);
  this->datagramV5.serialize(cursor);
}

uint32_t SampleDatagram::size() const {
  return 4 + this->datagramV5.size();
}

void SampledHeader::serialize(RWPrivateCursor* cursor) const {
  cursor->writeBE<uint32_t>(static_cast<uint32_t>(this->protocol));
  cursor->writeBE<uint32_t>(this->frameLength);
  cursor->writeBE<uint32_t>(this->stripped);
  cursor->writeBE<uint32_t>(this->headerLength);
  cursor->push(this->header, this->headerLength);
  if (this->headerLength % XDR_BASIC_BLOCK_SIZE > 0) {
    int fillCnt =
        XDR_BASIC_BLOCK_SIZE - this->headerLength % XDR_BASIC_BLOCK_SIZE;
    std::vector<byte> crud(XDR_BASIC_BLOCK_SIZE, 0);
    cursor->push(crud.data(), fillCnt);
  }
}

uint32_t SampledHeader::size() const {
  return 4 /* protocol */ + 4 /* frameLength */ + 4 /* stripped */ +
      4 /* headerLength */ + this->headerLength;
}

} // namespace facebook::fboss::sflow
