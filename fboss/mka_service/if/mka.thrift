// Copyright 2004-present Facebook. All Rights Reserved.

namespace cpp2 facebook.fboss.mka
namespace py3 neteng.fboss
namespace py neteng.fboss.mka
namespace py.asyncio neteng.fboss.asyncio.mka
namespace go neteng.fboss.mka

include "common/fb303/if/fb303.thrift"
include "fboss/mka_service/if/mka_config.thrift"
include "fboss/mka_service/if/mka_structs.thrift"
include "fboss/mka_service/if/mka_participant.thrift"

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

enum MacsecDirection {
  INGRESS = 0,
  EGRESS = 1,
}

exception MKAServiceException {
  1: MKAErrorResponseCode code;
  2: string msg;
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

enum MKAErrorLogType {
  INTERNAL_ERROR = 0,
  MKA_PARTICIPANT_ERROR = 1,
  MKPDU_ERROR = 2,
  MKA_HANDLER_ERROR = 3,
  MKA_SESSION_ERROR = 4,
  MKA_MODULE_ERROR = 5,
  MKA_SERIALIZER_ERROR = 6,
}

/*
 * This structure will be used to return the key hash values from the
 * interface
 */
struct KeyHash {
  // The CKN is send by called (key-server) while updating the profile on
  // interface. This will be typically the Unix timestamp when the key is
  // created (mka_config.MKAConfig.primaryCak.ckn).
  1: string ckn;
  // Hashed MacSec Key, the original key cannot be extracted
  2: string hashedKey;
}

/*
 * This structure returns macsec profile for an interface. The profile is set
 * by updateKey() function.
 */
struct InterfaceProfile {
  // Profile Name ie: Ethernet1234 (translates to eth1/23/4)
  1: string profileName;
  // Primary key hash
  2: KeyHash primaryKeyHash;
  // Seconday key hash
  3: KeyHash secondaryKeyHash;
  // Fallback key hash. The fallback key will eventually point to primary
  // or secondary key
  4: KeyHash fallbackKeyHash;
  // MKA timers (hello timer, life time of the key, sak timeout) in masec
  5: mka_config.MKATimers timers;
  // traffic policy for the profile, default to unknown
  6: mka_config.TrafficPolicy trafficPolicy = mka_config.TrafficPolicy.POLICY_UNKNOWN;
  // Cipher suite for the key
  7: mka_structs.MKACipherSuite cipherSuite;
}

/*
 * Status of the macsec key for a profile. This tells whether port is using
 * primary/fallback key or no key
 */
enum ProfileKeyStatus {
  // Unable to determine what is the key status
  KEY_UNKNOWN = 0,
  // Profile is not attached to the interface. Hence, there is no meaningful
  // key status
  KEY_NO_PROFILE = 1,
  // No key is being used
  KEY_NONE = 2,
  // Primary key is being used
  KEY_PRIMARY = 3,
  // Secondary key is being used
  KEY_SECONDARY = 4,
  // Fallback key is being used
  KEY_FALLBACK = 5,
}

/*
 * Macsec operational status of the profile on a given interface. This will
 * translate to if Macsec is enabled or not
 */
enum OperStatus {
  OPER_UNKNOWN = 0,
  OPER_NULL = 1,
  OPER_UP = 2,
  OPER_DOWN = 3,
}

/*
 * Struct returning the Macsec status of an interface
 */
struct EthernetStatus {
  // Profile name associated with a port. The profile name for the interface
  // "eth1/23/4" will be "Ethernet1234" if the macsec profile has been
  // created/updated for it earlier
  1: string profileName = "";
  // Currently we support cipher suite XPN_128 and XPN_256
  2: mka_structs.MKACipherSuite cipher = mka_structs.MKACipherSuite.GCM_AES_XPN_128;
  // Macsec profile's key status
  3: ProfileKeyStatus keyStatus = ProfileKeyStatus.KEY_UNKNOWN;
  // Macsec operational status
  4: OperStatus operStatus = OperStatus.OPER_UNKNOWN;
}

struct MKAPeerInfo {
  1: string id;
  2: i32 priority;
  3: bool sakUsed;
  4: string secureChannelIdentifier;
  5: bool isKeyServer;
}

struct MKASessionInfo {
  1: mka_participant.MKAParticipantCtx participantCtx;
  2: optional mka_participant.MKAParticipantCtx secondaryParticipantCtx;
  // Encrypted SAK key
  3: string encryptedSak;
  4: list<MKAPeerInfo> activePeersPrimary;
  5: list<MKAPeerInfo> potentialPeersPrimary;
  6: list<MKAPeerInfo> activePeersSecondary;
  7: list<MKAPeerInfo> potentialPeersSecondary;
}

/*
 * MKAService provides interface to the user or mka_service process through
 * thrift interface. The users are key_server and other test code. The
 * key_server will be the remote user and it will make thrift call using
 * inband and out of band management interface.
 */
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

  bool isSAKInstalled(1: string l2Port) throws (1: MKAServiceException ex) (
    cpp.coroutine,
  );

  list<string> getActivePorts() (cpp.coroutine);

  list<MKASessionInfo> getSessions() (cpp.coroutine);

  /*
   * This function creates/updates the macsec profile on a given interface. The
   * MKAConfig structure contains all the information like the profile name,
   * macsec key information etc. The port name is derived from profile name and
   * the macsec configuration is applied there. The update of the existing
   * profile configuration will lead to calling the create Macsec configuration
   * with updated parameters. If the profile does not exist already then also
   * it will lead to call for Create macsec configuration
   */
  void updateKey(
    // The MKAConfig structure for adding Macsec profile on the interface
    1: mka_config.MKAConfig update,
    // Hostname of the switch being queried
    101: string targetedRouterName,
  ) throws (1: MKAServiceException error);

  /*
   * This function removes the Macsec profile from a given interface. This
   * will result in calling the delete Macsec config for a port and remove
   * the profile from the port
   */
  void removeProfile(
    // Port name, ie "eth1/23/4"
    1: string interfaceName,
    // Hostname of the switch being queried
    101: string targetedRouterName,
  ) throws (1: MKAServiceException error);

  /*
   * This function returns the interface's Macsec profile information. The
   * returned struct contains all the Macsec config information of the profile
   * associated with the port.
   */
  InterfaceProfile getProfile(
    // Macsec profile name, ie: "Ethernet1234"
    1: string name,
    // Hostname of the switch being queried
    101: string targetedRouterName,
  ) throws (1: MKAServiceException error);

  /*
   * This function returns list of all the profiles existing on this switch.
   * The returned map contains the Interface profile info per profile name
   */
  map<string, InterfaceProfile> getAllProfiles(
    // Hostname of the switch being queried
    101: string targetedRouterName,
  ) throws (1: MKAServiceException error);

  /*
   * This function returns the port Macsec status for a given interface.
   */
  map<string, EthernetStatus> getEthernetStatus(
    // The interface name is in format like "eth1/23/4"
    1: string interfaceName,
    // The target router name is the hostname of the switch running
    // mka_service. This is just to validate the request
    101: string targetedRouterName,
  ) throws (1: MKAServiceException error);

  /*
   * This function returns the macsec status information of all the ports on
   * the given switch.
   */
  map<string, EthernetStatus> getAllEthernetStatus(
    // Hostname of the switch being queried
    101: string targetedRouterName,
  ) throws (1: MKAServiceException error);
}
