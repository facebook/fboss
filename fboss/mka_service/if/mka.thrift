// Copyright 2004-present Facebook. All Rights Reserved.

namespace cpp2 facebook.fboss.mka
namespace py3 neteng.fboss
namespace py neteng.fboss.mka
namespace py.asyncio neteng.fboss.asyncio.mka
namespace go neteng.fboss.mka

include "common/fb303/if/fb303.thrift"
include "fboss/mka_service/if/mka_config.thrift"

cpp_include "<unordered_set>"

// Response code specific to remote attestation.
enum MKAResponse {
  SUCCESS = 0,
  ERR_OPERATION_ERROR = 1,
  ERR_INVALID_INPUT = 2,
}

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

exception MKAServiceException {
  1: MKAErrorResponseCode code;
  2: string msg;
}

struct MKASci {
  1: string macAddress;
  2: i32 port = 1;
}

struct MKASak {
  1: MKASci sci;
  2: string l2Port;
  3: i16 assocNum = 0;
  4: string keyHex;
  5: string keyIdHex;
  6: mka_config.MKAConfidentialityOffset confidentOffset = mka_config.MKAConfidentialityOffset.CONFIDENTIALITY_NO_OFFSET;
  7: bool primary = false;
}

struct MKAActiveSakSession {
  1: MKASak sak;
  2: list<MKASci> sciList;
}

struct MKASakHealthResponse {
  1: bool active;
  // 32 bit LPN
  2: i64 lowestAcceptablePN;
}

struct MacsecPortPhyInfo {
  1: i32 slotId
  2: i32 mdioId
  3: i32 phyId
}

struct MacsecPortPhyMap {
  1: map<i32, MacsecPortPhyInfo> macsecPortPhyMap;
}

struct MacsecScInfo {
  1: i32 scId
  2: i32 flowId
  3: i32 aclId
  4: bool directionIngress
  5: list<i32> saList
}

struct MacsecPortInfo {
  1: i32 ingressPort
  2: i32 egressPort
}

struct MacsecAllScInfo {
  1: list<MacsecScInfo> scList
  2: map<i32, MacsecPortInfo> linePortInfo
}

struct MacsecPortStats {
  1: i64 preMacsecDropPkts
  2: i64 controlPkts
  3: i64 dataPkts
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

service MKAService extends fb303.FacebookService {
  // provisionCAK
  MKAResponse provisionCAK(1: mka_config.MKAConfig config) throws (
    1: MKAServiceException ex,
  ) (cpp.coroutine);

  // get total number of active mka sessions
  i32 numActiveMKASessions() (cpp.coroutine);

  string getActiveCKN(1: string l2Port) throws (1: MKAServiceException ex) (
    cpp.coroutine,
  );

  list<string> getActivePorts() (cpp.coroutine);
}
