namespace cpp2 facebook.fboss.mka
namespace py neteng.fboss.mka.mka_structs
namespace py3 neteng.fboss.mka
namespace py.asyncio neteng.fboss.asyncio.mka_structs
namespace go neteng.fboss.mka.mka_structs

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

struct MacsecPortStats {
  1: i64 preMacsecDropPkts;
  2: i64 controlPkts;
  3: i64 dataPkts;
}

struct MacsecFlowStats {
  1: bool directionIngress;
  2: i64 ucastUncontrolledPkts;
  3: i64 ucastControlledPkts;
  4: i64 mcastUncontrolledPkts;
  5: i64 mcastControlledPkts;
  6: i64 bcastUncontrolledPkts;
  7: i64 bcastControlledPkts;
  8: i64 controlPkts;
  9: i64 untaggedPkts;
  10: i64 otherErrPkts;
  11: i64 octetsUncontrolled;
  12: i64 octetsControlled;
  13: i64 outCommonOctets;
  14: i64 outTooLongPkts;
  15: i64 inTaggedControlledPkts;
  16: i64 inUntaggedPkts;
  17: i64 inBadTagPkts;
  18: i64 noSciPkts;
  19: i64 unknownSciPkts;
  20: i64 overrunPkts;
}

struct MacsecSaStats {
  1: bool directionIngress;
  2: i64 octetsEncrypted;
  3: i64 octetsProtected;
  4: i64 outEncryptedPkts;
  5: i64 outProtectedPkts;
  6: i64 inUncheckedPkts;
  7: i64 inDelayedPkts;
  8: i64 inLatePkts;
  9: i64 inInvalidPkts;
  10: i64 inNotValidPkts;
  11: i64 inNoSaPkts;
  12: i64 inUnusedSaPkts;
  13: i64 inOkPkts;
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
}

struct MKAActiveSakSession {
  1: MKASak sak;
  2: list<MKASci> sciList;
}
