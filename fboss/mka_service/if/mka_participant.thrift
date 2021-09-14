// Copyright 2004-present Facebook. All Rights Reserved.

namespace cpp2 facebook.fboss.mka
namespace py3 neteng.fboss.mka
namespace py neteng.fboss.mka.mka_participant

include "fboss/mka_service/if/mka_config.thrift"
include "fboss/mka_service/if/mka_structs.thrift"

cpp_include "folly/io/IOBuf.h"

struct MKAParticipantCtx {
  1: mka_structs.Cak cak;
  2: mka_structs.MKAConfidentialityOffset confidentOffset = mka_structs.MKAConfidentialityOffset.CONFIDENTIALITY_NO_OFFSET;
  // When distSAK_ is set, then participant is the keyserver
  // and when mkpdu is sent, we will distribute the sak.
  // when all the peers have consumed the sak then we'll reset this flag.
  3: bool distSAK = false;
  // if we are keyserver we should send the dist sak irrespective
  // of SAKUse from the peer.
  4: bool keyServerDistSAK = false;
  // set when the keyserver election is done
  5: bool elected = false;
  // when sak is enabled on rx
  6: bool sakEnabledRx = false;
  // when sak is enabled on tx
  7: bool sakEnabledTx = false;
  8: bool delayProtect = false;
  9: bool plainTx = false;
  10: bool plainRx = false;
  11: i64 sakLifeTime = 0;
  12: i64 peerLifeTime = 0;
  13: string ick;
  14: string kek;
  15: string l2Port;
  16: mka_structs.MKACipherSuite cipher = mka_structs.MKACipherSuite.GCM_AES_XPN_128;
  17: string srcMac;
  18: bool valid = false;
}

struct MKAParticipantIOBufs {
  // store the cpp structures as IOBufs for binary serialize/deserialize.
  // we need to store this ctx for warm reboot so the service can start
  // sending MkPdu for existing sessions.
  1: binary (cpp2.type = "folly::IOBuf") actor;
  2: binary (cpp2.type = "folly::IOBuf") latestKeyServer;
  3: binary (cpp2.type = "folly::IOBuf") oldKeyServer;
  4: binary (cpp2.type = "folly::IOBuf") livePeers;
  5: binary (cpp2.type = "folly::IOBuf") potentialPeers;
}

struct MKASessionConfig {
  1: i32 version;
  // magic string to find corrupted configs.
  2: string magic;
  3: i64 timestamp;
  4: mka_config.MKAConfig mkaConfig;
  5: MKAParticipantCtx primaryParticipant;
  6: MKAParticipantIOBufs primaryParticipantIOBufs;
  7: optional MKAParticipantCtx secondaryParticipant;
  8: optional MKAParticipantIOBufs secondaryParticipantIOBufs;
}

struct MKASakListConfig {
  1: i32 version;
  // magic string to find corrupted configs.
  2: string magic;
  3: i64 timestamp;
  4: list<mka_structs.MKAActiveSakSession> sakList;
}
