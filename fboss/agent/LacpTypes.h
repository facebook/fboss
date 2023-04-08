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

#include <array>
#include <chrono>
#include <string>

#include <folly/MacAddress.h>
#include <folly/Range.h>
#include <folly/dynamic.h>
#include <folly/io/Cursor.h>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"

namespace facebook::fboss {

class LacpEndpoint;

static const uint16_t kDefaultSystemPriority = 65535;

class LACPError : public FbossError {
 public:
  template <typename... Args>
  explicit LACPError(Args&&... args)
      : FbossError(std::forward<Args>(args)...) {}

  ~LACPError() throw() override {}
};

enum LacpState : int {
  NONE = 0x00,
  LACP_ACTIVE = 0x01,
  SHORT_TIMEOUT = 0x02,
  AGGREGATABLE = 0x04,
  IN_SYNC = 0x08,
  COLLECTING = 0x10,
  DISTRIBUTING = 0x20,
  DEFAULTED = 0x40,
  EXPIRED = 0x80
};

inline LacpState operator|=(LacpState& lhs, const LacpState rhs) {
  return (
      lhs = static_cast<LacpState>(
          static_cast<int>(lhs) | static_cast<int>(rhs)));
}

inline LacpState operator&=(LacpState& lhs, const LacpState rhs) {
  return (
      lhs = static_cast<LacpState>(
          static_cast<int>(lhs) & static_cast<int>(rhs)));
}

inline LacpState operator~(const LacpState& s) {
  return static_cast<LacpState>(~static_cast<int>(s));
}

inline LacpState operator|(const LacpState& lhs, const LacpState& rhs) {
  return static_cast<LacpState>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline LacpState operator&(const LacpState& lhs, const LacpState& rhs) {
  return static_cast<LacpState>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

struct ParticipantInfo {
  using SystemPriority = uint16_t;
  using SystemID = std::array<uint8_t, 6>;
  using Key = uint16_t;
  using PortPriority = uint16_t;
  using Port = uint16_t;

  SystemPriority systemPriority{0};
  SystemID systemID{{0, 0, 0, 0, 0, 0}};
  Key key{0};
  PortPriority portPriority{0};
  Port port{0};
  LacpState state{LacpState::NONE};

  std::string describe() const;

  template <typename CursorType>
  static ParticipantInfo from(CursorType* cursor);
  static ParticipantInfo defaultParticipantInfo();

  template <typename CursorType>
  void to(CursorType* cursor) const;

  void populate(LacpEndpoint& endpoint) const;

  folly::dynamic toFollyDynamic() const;
  static ParticipantInfo fromFollyDynamic(const folly::dynamic& json);
  state::ParticipantInfo toThrift() const;
  static ParticipantInfo fromThrift(const state::ParticipantInfo& data);

  bool operator==(const ParticipantInfo& rhs) const;
  bool operator!=(const ParticipantInfo& rhs) const;
};

// TODO(samank): different structs for rx and tx lacpdus?
struct LACPDU {
  // LENGTH includes enough space for a .1q tagged Ethernet header and the LACP
  // PDU. Note that no room is allocated for the Ethernet FCS because it will
  // be appened to the frame in the ASIC.
  enum { LENGTH = 0x80 };
  enum EtherType : uint16_t { SLOW_PROTOCOLS = 0x8809 };
  enum EtherSubtype : uint8_t { LACP = 0x01, MARKER = 0x02 };

  using Version = uint8_t;
  using TLVType = uint8_t;
  using TLVLength = uint8_t;
  using Delay = uint16_t;

  LACPDU() {}
  LACPDU(ParticipantInfo actorInfo, ParticipantInfo partnerInfo);

  bool isValid() const;

  template <typename CursorType>
  static LACPDU from(CursorType* c);
  template <typename CursorType>
  void to(CursorType* c) const;

  std::string describe() const;

  static const folly::MacAddress& kSlowProtocolsDstMac() {
    static const folly::MacAddress slowProtocolsDstMac("01:80:c2:00:00:02");
    return slowProtocolsDstMac;
  }

  Version version{0x01};

  TLVType actorType{0x01};
  TLVLength actorInfoLength{0x14};
  ParticipantInfo actorInfo;
  std::array<std::uint8_t, 3> actorReserved{};

  TLVType partnerType{0x02};
  TLVLength partnerInfoLength{0x14};
  ParticipantInfo partnerInfo;
  std::array<std::uint8_t, 3> partnerReserved{};

  TLVType collectorType{0x03};
  TLVLength collectorLength{0x10};
  Delay maxDelay{0x0000};
  std::array<std::uint8_t, 12> collectorReserved{};

  TLVType terminatorType{0x00};
  TLVLength terminatorLength{0x00};
  std::array<std::uint8_t, 50> terminatorReserved{};
};

class LinkAggregationGroupID {
 public:
  LinkAggregationGroupID() {}

  static LinkAggregationGroupID from(
      const ParticipantInfo& actorInfo,
      const ParticipantInfo& partnerInfo);

  std::string describe() const;

  bool operator==(const LinkAggregationGroupID& rhs) const;

  ParticipantInfo::SystemID actorSystemID{};
  ParticipantInfo::SystemID partnerSystemID{};
  ParticipantInfo::SystemPriority actorSystemPriority{0};
  ParticipantInfo::SystemPriority partnerSystemPriority{0};
  ParticipantInfo::Key actorKey{0};
  ParticipantInfo::Key partnerKey{0};
  ParticipantInfo::Port actorPort{0};
  ParticipantInfo::Port partnerPort{0};
  ParticipantInfo::PortPriority actorPortPriority{0};
  ParticipantInfo::PortPriority partnerPortPriority{0};
};

} // namespace facebook::fboss
