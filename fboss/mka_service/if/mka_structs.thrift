namespace cpp2 facebook.fboss.mka
namespace py neteng.fboss.mka.mka_structs
namespace py3 neteng.fboss.mka
namespace py.asyncio neteng.fboss.asyncio.mka_structs
namespace go neteng.fboss.mka.mka_structs
namespace php fboss

enum MKAErrorResponseCode {
  INVALID_CKN = 0,
  INVALID_INTERFACE = 1,
  INVALID_L2PORT = 2,
  L2PORT_NOT_FOUND = 3,
  UPDATE_SESSION_FAILED = 4,
  INTERNAL_ERROR = 5,
  HANDLER_NOT_INITED = 6,
  INVALID_INPUT = 7,
}

// Response code specific to remote attestation.
enum MKAResponse {
  SUCCESS = 0,
  ERR_OPERATION_ERROR = 1,
  ERR_INVALID_INPUT = 2,
}

enum MKAErrorLogType {
  INTERNAL_ERROR = 0,
  MKA_PARTICIPANT_ERROR = 1,
  MKPDU_ERROR = 2,
  MKA_HANDLER_ERROR = 3,
  MKA_SESSION_ERROR = 4,
  MKA_MODULE_ERROR = 5,
  MKA_SERIALIZER_ERROR = 6,
}

exception MKAServiceException {
  1: MKAErrorResponseCode code;
  2: string msg;
}

enum MacsecDirection {
  INGRESS = 0,
  EGRESS = 1,
}

struct MKASakHealthResponse {
  1: bool active;
  // 32 bit LPN
  2: i64 lowestAcceptablePN;
}

struct MacsecPortPhyInfo {
  1: i32 slotId;
  2: i32 mdioId;
  3: i32 phyId;
  4: i32 saiSwitchId;
  5: string portName;
}

struct MacsecPortPhyMap {
  1: map<i32, MacsecPortPhyInfo> macsecPortPhyMap;
}

struct MacsecScInfo {
  1: i32 scId;
  2: i32 flowId;
  3: i32 aclId;
  4: bool directionIngress;
  5: list<i32> saList;
}

struct MacsecPortInfo {
  1: i32 ingressPort;
  2: i32 egressPort;
}

struct MacsecAllScInfo {
  1: list<MacsecScInfo> scList;
  2: map<i32, MacsecPortInfo> linePortInfo;
}

enum PortAdminStatus {
  NO_MACSEC = 0,
  MACSEC_WITH_PLAINTEXT_FALLBACK = 1,
  MACSEC_NO_FALLBACK = 2,
}

struct MacsecPortStats {
  1: i64 preMacsecDropPkts;
  2: i64 controlPkts;
  3: i64 dataPkts;
  // Derived counters, extracted from SA, flow counters
  // For macsec stats behavior refer 802.1AE IEEE Std - 2006, chapter 10,
  // figure 10-5.
  4: i64 octetsEncrypted = 0; // from SA.octetsEncrypted
  // In errors - counters with Dropped in their name reflect
  // packets dropped, while others reflect MASEC errors that
  // are firther passed on for ICV or ASIC processing
  // Behaivior is defined in
  5: i64 inBadOrNoMacsecTagDroppedPkts = 0; // from Flow.{inNoTagPkts + inBadTagPkts}
  6: i64 inNoSciDroppedPkts = 0; // from Flow.noSciPkts
  7: i64 inUnknownSciPkts = 0; // from Flow.unknownSciPkts
  8: i64 inOverrunDroppedPkts = 0; // from Flow.overrunPkts
  9: i64 inDelayedPkts = 0; // from SA.inDelayedPkts
  10: i64 inLateDroppedPkts = 0; // from SA.inLatePkts
  11: i64 inNotValidDroppedPkts = 0; // from SA.inNotValidPkts
  12: i64 inInvalidPkts = 0; // from SA.inInvalidPkts
  13: i64 inNoSaDroppedPkts = 0; // from SA.inNoSaPkts
  14: i64 inUnusedSaPkts = 0; // from SA.inUnusedSaPkts
  15: i64 inCurrentXpn = 0; // from SA.inCurrentXpn
  // Out Errors
  30: i64 outTooLongDroppedPkts = 0; // from Flow.outTooLongPkts
  31: i64 outCurrentXpn = 0; // from SA.outCurrentXpn
  // In/Out error counters
  45: i64 noMacsecTagPkts = 0; // from Flow.untaggedPkts
}
/*
 Macsec stats behavior is explained in 802.1AE IEEE Std - 2006, chapter 10,
 figure 10-5. Inline comments below briefly explain counter semantics Tag here
 refers to MACSEC frame SecTag ICV - integrity check validation PN - packet
 number Controlled port - Port with MACSEC protection (ICV) and optionally
 encryption FBOSS always programs with both protection and encryption
 Uncontrolled port - unsecured port
 */
struct MacsecFlowStats {
  1: bool directionIngress;
  // ucast, mcast, bcast (in, out) packets on (uncontrolled, controlled) ports
  2: i64 ucastUncontrolledPkts;
  3: i64 ucastControlledPkts;
  4: i64 mcastUncontrolledPkts;
  5: i64 mcastControlledPkts;
  6: i64 bcastUncontrolledPkts;
  7: i64 bcastControlledPkts;
  // Control pkts that bypass macsec security
  8: i64 controlPkts;
  // in (dir = ingress) or out (dir == egress) untgaged packets
  9: i64 untaggedPkts;
  10: i64 otherErrPkts;
  // in/out octets on uncontrolled port
  11: i64 octetsUncontrolled;
  // in/out octets on controlled port
  12: i64 octetsControlled;
  // out octets on common port
  13: i64 outCommonOctets;
  // packet_len > common_port->max_len
  14: i64 outTooLongPkts;
  /*
  Ingress only stats, 0 for egress
  */
  // In packets with MACSEC Tag
  15: i64 inTaggedControlledPkts;
  // In packets w/o MACSEC Tag (validate == Strict)
  // 802.1AE also defines inUntaggedPkts for in packets w/o
  // macsec tag and (validate != Strict). But SAI does not support
  // it
  16: i64 inNoTagPkts;
  // Invalid tag or ICV fail
  17: i64 inBadTagPkts;
  // SCI not found and validate == Strict
  18: i64 noSciPkts;
  // SCI not found and validate != Strict
  19: i64 unknownSciPkts;
  // In packets exceeds capabilities of Cipher suite
  20: i64 overrunPkts;
}

struct MacsecSaStats {
  1: bool directionIngress;
  // Octets encrypted/decrypted by controlled port
  2: i64 octetsEncrypted;
  // Octets on controlled port that were
  // Egress - not encrypted, but protected (by say integrity check validation)
  // Ingress - not decrypted but validated
  3: i64 octetsProtected;
  4: i64 outEncryptedPkts;
  5: i64 outProtectedPkts;
  // Validation disabled pkts
  6: i64 inUncheckedPkts;
  // Replay protection OFF and packet number (PN) < sa->lowestAcceptable_PN
  7: i64 inDelayedPkts;
  // Replay protection ON and packet number (PN) < sa->lowestAcceptable_PN
  8: i64 inLatePkts;
  // Pkt failed validation and validateFrames != Strict
  9: i64 inInvalidPkts;
  // Pkt failed validation and validateFrames == Strict
  10: i64 inNotValidPkts;
  // SA not found and validate != strict
  11: i64 inNoSaPkts;
  // SA not found and validate == strict
  12: i64 inUnusedSaPkts;
  // In packets that passed all checks on a controlled port
  13: i64 inOkPkts;
  // Current egress extended packet number
  14: i64 outCurrentXpn;
  // Current ingress extended packet number
  15: i64 inCurrentXpn;
}

struct MacsecAclStats {
  1: i64 defaultAclStats;
  2: optional i64 controlAclStats;
  3: optional i64 flowAclStats;
}

enum MKACipherSuite {
  GCM_AES_128 = 0,
  GCM_AES_256 = 1,
  GCM_AES_XPN_128 = 2,
  GCM_AES_XPN_256 = 3,
}

// IEEE 8021X.2010 TABLE 11.6
enum MKAConfidentialityOffset {
  CONFIDENTIALITY_NOT_USED = 0,
  CONFIDENTIALITY_NO_OFFSET = 1,
  CONFIDENTIALITY_OFFSET_30 = 2,
  CONFIDENTIALITY_OFFSET_50 = 3,
}

struct MKASci {
  1: string macAddress;
  2: i32 port = 1;
}

struct MKASecureAssociationId {
  1: MKASci sci;
  2: i16 assocNum;
}

struct MKASak {
  1: MKASci sci;
  2: string l2Port;
  3: i16 assocNum = 0;
  4: string keyHex;
  5: string keyIdHex;
  6: mka_structs.MKAConfidentialityOffset confidentOffset = mka_structs.MKAConfidentialityOffset.CONFIDENTIALITY_NO_OFFSET;
  7: bool primary = false;
  8: bool dropUnencrypted = true;
}

struct MKAActiveSakSession {
  1: MKASak sak;
  2: list<MKASci> sciList;
}
