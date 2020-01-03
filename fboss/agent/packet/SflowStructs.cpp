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

using namespace folly;
using namespace folly::io;

namespace facebook::fboss {

namespace sflow {

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
  cursor->writeBE<uint32_t>(this->flowDataLen);
  cursor->push(this->flowData, this->flowDataLen);
  if (this->flowDataLen % XDR_BASIC_BLOCK_SIZE != 0) {
    int fillCnt =
        XDR_BASIC_BLOCK_SIZE - this->flowDataLen % XDR_BASIC_BLOCK_SIZE;
    std::vector<byte> crud(XDR_BASIC_BLOCK_SIZE, 0);
    cursor->push(crud.data(), fillCnt);
  }
}

uint32_t FlowRecord::size() const {
  return 4 /* flowFormat */ + 4 /* flowDataLen */ + this->flowDataLen;
}

void FlowSample::serialize(RWPrivateCursor* cursor) const {
  cursor->writeBE<uint32_t>(this->sequenceNumber);
  serializeSflowDataSource(cursor, this->sourceID);

  cursor->writeBE<uint32_t>(this->samplingRate);
  cursor->writeBE<uint32_t>(this->samplePool);
  cursor->writeBE<uint32_t>(this->drops);
  serializeSflowPort(cursor, this->input);
  serializeSflowPort(cursor, this->output);
  cursor->writeBE<uint32_t>(this->flowRecordsCnt);
  for (int i = 0; i < this->flowRecordsCnt; i++) {
    this->flowRecords[i].serialize(cursor);
  }
}

uint32_t FlowSample::size(const uint32_t frecordsSize) const {
  // TODO infer the flow records size instead and remove the input param.
  return 4 /* sequenceNumber */ + 4 /* sourceId */ + 4 /* samplingRate */ +
      4 /* samplePool */ + 4 /* drops */ + 4 /* input */ + 4 /* output */ +
      4 /* flowRecordCnt */ + frecordsSize;
}

void SampleRecord::serialize(RWPrivateCursor* cursor) const {
  serializeDataFormat(cursor, this->sampleType);
  cursor->writeBE<uint32_t>(this->sampleDataLen);
  // Serialize XDR opaque sFlow sample_data
  cursor->push(this->sampleData, this->sampleDataLen);
  if (this->sampleDataLen % XDR_BASIC_BLOCK_SIZE > 0) {
    int fillCnt =
        XDR_BASIC_BLOCK_SIZE - this->sampleDataLen % XDR_BASIC_BLOCK_SIZE;
    std::vector<byte> crud(XDR_BASIC_BLOCK_SIZE, 0);
    cursor->push(crud.data(), fillCnt);
  }
}

uint32_t SampleRecord::size() const {
  return 4 /* sampleType */ + 4 /* sampleDataLen */ + this->sampleDataLen;
}

void SampleDatagramV5::serialize(RWPrivateCursor* cursor) const {
  serializeIP(cursor, this->agentAddress);

  cursor->writeBE<uint32_t>(this->subAgentID);
  cursor->writeBE<uint32_t>(this->sequenceNumber);
  cursor->writeBE<uint32_t>(this->uptime);
  cursor->writeBE<uint32_t>(this->samplesCnt);
  for (int i = 0; i < this->samplesCnt; i++) {
    this->samples[i].serialize(cursor);
  }
}

uint32_t SampleDatagramV5::size(const uint32_t recordsSize) const {
  return this->agentAddress.byteCount() + 4 /* subAgentID */ +
      4 /*sequenceNumber */ + 4 /*uptime*/
      + 4 /*samplesCnt */ + recordsSize;
}

void SampleDatagram::serialize(RWPrivateCursor* cursor) const {
  cursor->writeBE<uint32_t>(SampleDatagram::VERSION5);
  this->datagramV5.serialize(cursor);
}

uint32_t SampleDatagram::size(const uint32_t recordsSize) const {
  return 4 + this->datagramV5.size(recordsSize);
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

} // namespace sflow

} // namespace facebook::fboss
