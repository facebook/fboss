/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <variant>
#include <vector>

#include <folly/ExceptionString.h>
#include <folly/IPAddress.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>

namespace facebook::fboss::sflow {

constexpr int XDR_BASIC_BLOCK_SIZE = 4; // XDR basic block size is 4 bytes

// aliases
using byte = uint8_t;

/*
 *  Below is the (partial) implementation of the structs representing the
 *  sFlow Datagram Format as defined in Section 5 of the sFlow V5 spec.
 *  Please refer to http://www.sFlow.org/ for more information.
 */

enum struct AddressType : uint32_t { UNKNOWN = 0, IP_V4 = 1, IP_V6 = 2 };

void serializeIP(folly::io::RWPrivateCursor* cursor, folly::IPAddress ip);
folly::IPAddress deserializeIP(folly::io::Cursor& cursor);
uint32_t sizeIP(folly::IPAddress ip);

/* Data Format */
using DataFormat = uint32_t;
void serializeDataFormat(folly::io::RWPrivateCursor* cursor, DataFormat fmt);

/* sFlowDataSource */
using SflowDataSource = uint32_t;
void serializeSflowDataSource(
    folly::io::RWPrivateCursor* cursor,
    SflowDataSource src);

/* Input/output port information */
using SflowPort = uint32_t;
void serializeSflowPort(
    folly::io::RWPrivateCursor* cursor,
    SflowPort sflowPort);

/* Proposed standard sFlow data formats (draft 14) */
/* Packet Header Data */
/* header_potocol enumeration */
enum struct HeaderProtocol : uint32_t {
  ETHERNET_ISO88023 = 1,
  ISO88024_TOKENBUS = 2,
  ISO88025_TOKENRING = 3,
  FDDI = 4,
  FRAME_RELAY = 5,
  X25 = 6,
  PPP = 7,
  SMDS = 8,
  AAL5 = 9,
  AAL5_IP = 10,
  IPV4 = 11,
  IPV6 = 12,
  MPLS = 13,
  POS = 14
};

/* Raw Packet Header */
/* opaque = flow_data; enterprice = 0; format = 1 */
struct SampledHeader {
  HeaderProtocol protocol{};
  uint32_t frameLength{};
  uint32_t stripped{};
  std::vector<uint8_t> header{};

  void serialize(folly::io::RWPrivateCursor* cursor) const;
  uint32_t size() const;
  static SampledHeader deserialize(folly::io::Cursor& cursor);
};

/* Flow data variant - supports different types of flow data */
using FlowData = std::variant<SampledHeader>;

/* Flow record format */
struct FlowRecord {
  DataFormat flowFormat;
  FlowData flowData;

  void serialize(folly::io::RWPrivateCursor* cursor) const;
  uint32_t size() const;
  static FlowRecord deserialize(folly::io::Cursor& cursor);
};

// TODO (sgwang)
// struct counter_record {...}

/* Compact Format Flow/Counter samples
 * If ifindex numbers are always < 2^24 then the compact must be used */

/* Format of a single flow sample */
/* opaque = sample_data; enterprise = 0; format = 1 */
struct FlowSample {
  uint32_t sequenceNumber;
  SflowDataSource sourceID;
  uint32_t samplingRate;
  uint32_t samplePool;
  uint32_t drops;
  SflowPort input;
  SflowPort output;
  std::vector<FlowRecord> flowRecords;

  void serialize(folly::io::RWPrivateCursor* cursor) const;
  uint32_t size() const;
  static FlowSample deserialize(folly::io::Cursor& cursor);
};

/* Format of a single counter sample */
/* opaque = sample_data; enterprise = 0; format = 2 */
// TODO (sgwang)
// struct counters_sample {...}

/* Extended Format Flow/Counter samples
 * If ifindex numbers may be >= 2^24 then the expanded must be used */
// TODO (sgwang)
// struct sflow_data_source_expanded {...}
// struct interface_expanded {...}

/* Format of a single expanded flow sample */
/* opaque = sample_data; enterprise = 0; format = 3 */
// TODO (sgwang)
// struct flow_sample_expanded {...}

/* Format of a single expanded counter sample */
/* opaque = sample_data; enterprise = 0; format = 4 */
// TODO (sgwang)
// struct counters_sample_expanded {...}

/* Sample data variant - supports different types of sample data */
using SampleData = std::variant<FlowSample>;

/* Format of a sample datagram */
struct SampleRecord {
  DataFormat sampleType;
  std::vector<SampleData> sampleData;

  void serialize(folly::io::RWPrivateCursor* cursor) const;
  uint32_t size() const;
  static SampleRecord deserialize(folly::io::Cursor& cursor);
};

/* Header information for sFlow version 5 datagrams */
struct SampleDatagramV5 {
  folly::IPAddress agentAddress;
  uint32_t subAgentID;
  uint32_t sequenceNumber; // for sub-agent
  uint32_t uptime;
  std::vector<SampleRecord> samples;

  void serialize(folly::io::RWPrivateCursor* cursor) const;
  uint32_t size() const;
  static SampleDatagramV5 deserialize(folly::io::Cursor& cursor);
};

// Here we skip sample_datagram_type, since only v5 is used
struct SampleDatagram {
  // We only consider sFlowV5
  static constexpr uint32_t VERSION5 = 5;
  SampleDatagramV5 datagramV5;

  void serialize(folly::io::RWPrivateCursor* cursor) const;
  uint32_t size() const;
  static SampleDatagram deserialize(folly::io::Cursor& cursor);
};

// .. We omit the spec definition below (including) "Ethernet Frame Data" on p36

} // namespace facebook::fboss::sflow
