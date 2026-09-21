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

#include "fboss/agent/LacpTypes.h"

namespace facebook::fboss {

// TODO(samank): less verbose&cheaper parsing
// TODO(samank): early&cheaper bounds checking
// TOOD(samank): factor out value parsing
template <typename CursorType>
ParticipantInfo ParticipantInfo::from(CursorType* cursor) {
  ParticipantInfo info;

  info.systemPriority =
      cursor->template readBE<ParticipantInfo::SystemPriority>();
  cursor->pull(info.systemID.begin(), info.systemID.size());
  info.key = cursor->template readBE<ParticipantInfo::Key>();
  info.portPriority = cursor->template readBE<ParticipantInfo::PortPriority>();
  info.port = cursor->template readBE<ParticipantInfo::Port>();
  info.state = static_cast<LacpState>(cursor->template readBE<uint8_t>());

  return info;
}

template <typename CursorType>
void ParticipantInfo::to(CursorType* cursor) const {
  cursor->template writeBE<SystemPriority>(systemPriority);
  cursor->push(systemID.begin(), systemID.size());
  cursor->template writeBE<Key>(key);
  cursor->template writeBE<PortPriority>(portPriority);
  cursor->template writeBE<Port>(port);
  cursor->template writeBE<uint8_t>(static_cast<uint8_t>(state));
}

template <typename CursorType>
LACPDU LACPDU::from(CursorType* c) {
  LACPDU lacpdu;

  lacpdu.version = c->template readBE<LACPDU::Version>();

  lacpdu.actorType = c->template readBE<LACPDU::TLVType>();
  lacpdu.actorInfoLength = c->template readBE<LACPDU::TLVLength>();
  lacpdu.actorInfo = ParticipantInfo::from(c);

  *c += 3; // r1

  lacpdu.partnerType = c->template readBE<LACPDU::TLVType>();
  lacpdu.partnerInfoLength = c->template readBE<LACPDU::TLVLength>();
  lacpdu.partnerInfo = ParticipantInfo::from(c);

  *c += 3; // r2

  lacpdu.collectorType = c->template readBE<LACPDU::TLVType>();
  lacpdu.collectorLength = c->template readBE<LACPDU::TLVLength>();

  lacpdu.maxDelay = c->template readBE<LACPDU::Delay>();

  *c += 12; // r3

  lacpdu.terminatorType = c->template readBE<LACPDU::TLVType>();
  lacpdu.terminatorLength = c->template readBE<LACPDU::TLVLength>();

  *c += 50; // r4

  return lacpdu;
}

template <typename CursorType>
void LACPDU::to(CursorType* c) const {
  c->template writeBE<LACPDU::Version>(version);

  c->template writeBE<LACPDU::TLVType>(actorType);
  c->template writeBE<LACPDU::TLVLength>(actorInfoLength);
  actorInfo.to(c);

  c->push(folly::ByteRange(actorReserved.begin(), actorReserved.end()));

  c->template writeBE<LACPDU::TLVType>(partnerType);
  c->template writeBE<LACPDU::TLVLength>(partnerInfoLength);
  partnerInfo.to(c);

  c->push(folly::ByteRange(partnerReserved.begin(), partnerReserved.end()));

  c->template writeBE<LACPDU::TLVType>(collectorType);
  c->template writeBE<LACPDU::TLVLength>(collectorLength);
  c->template writeBE<LACPDU::Delay>(maxDelay);

  c->push(folly::ByteRange(collectorReserved.begin(), collectorReserved.end()));

  c->template writeBE<LACPDU::TLVType>(terminatorType);
  c->template writeBE<LACPDU::TLVLength>(terminatorLength);

  c->push(
      folly::ByteRange(terminatorReserved.begin(), terminatorReserved.end()));
}

} // namespace facebook::fboss
