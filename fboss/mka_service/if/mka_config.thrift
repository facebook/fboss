namespace cpp2 facebook.fboss.mka
namespace py neteng.fboss.mka.mka_config
namespace py3 neteng.fboss.mka
namespace py.asyncio neteng.fboss.asyncio.mka_config
namespace go neteng.fboss.mka.mka_config

include "fboss/mka_service/if/mka_structs.thrift"

const i16 DEFAULT_KEYSERVER_PRIORITY = 0x16;

// All the timeouts are expected in milliseconds
const i64 DEFAULT_SAK_TIMEOUT_IN_MSEC = 0; // never expires
// default TIMEOUT Ref: IEEE8021x-2010 Table 9-3
const i16 DEFAULT_HELLO_TIMEOUT_IN_MSEC = 2000;
const i16 DEFAULT_LIFE_TIMEOUT_IN_MSEC = 6000;

enum MKATransport {
  LINUX_PACKET_TRANSPORT = 0,
  THRIFT_TRANSPORT = 1,
  // INVALID
  INVALID = 100,
}

enum MACSecCapability {
  CAPABILITY_NOT_IMPLEMENTED = 0,
  CAPABILITY_INTEGRITY = 1,
  CAPABILITY_INGTY_CONF = 2,
  CAPABILITY_INGTY_CONF_30_50 = 3,
}

// All the timeouts are specified in milli seconds.
struct MKATimers {
  1: i16 helloTime = DEFAULT_HELLO_TIMEOUT_IN_MSEC;
  2: i16 lifeTime = DEFAULT_LIFE_TIMEOUT_IN_MSEC;
  3: i64 sakTime = DEFAULT_SAK_TIMEOUT_IN_MSEC; // sak expriation timeout
}

/*
 * Traffic policy tells about the interface macsec status - if it has active
 * SA to encrypt/decrypt traffic or some other state
 */
enum TrafficPolicy {
  // Unable to determine traffic policy (init state)
  POLICY_UNKNOWN = 0,
  // Profile Not set
  POLICY_NULL = 1,
  // allow transmit/receipt of encrypted traffic using operation SAK,
  // block otherwise
  POLICY_ACTIVE_SAK = 2,
  // Allow transmit/receipt of unprotected traffic (macsec bypass)
  POLICY_UNPROTECTED = 3,
  // block transmit/receipt of unprotected traffic
  POLICY_BLOCKED = 4,
}

/*
 * Key Type will be used by Key Server to tell whether to program primary key
 * or the secondary key
 */
enum KeyType {
  // Primary key, this will be active
  PRIMARY_KEY = 0,
  // Secondary key is for backup or rollover
  SECONDARY_KEY = 1,
}

/*
 * Struct used to keep the Macsec config for a port. Each port will have a
 * different Macsec config as defined in the config file or as passed from
 * KeyServer thrift
 */
struct MKAConfig {
  // This is FBOSS swicth ethernet port name string like: eth4/2/1
  1: string l2Port;
  // Source MAC of MKA session endpoints
  2: string srcMac;
  // The maximum priority value should be 255.
  // The datastructure here should be uint8_t (0-255)
  3: optional i16 priority = DEFAULT_KEYSERVER_PRIORITY;
  // Primary CAK is the active one
  4: mka_structs.Cak primaryCak;
  // Secondary CAK will be used for Backup or fallback CAK
  5: optional mka_structs.Cak secondaryCak;
  // Transport endpoint is Thrift for production, for UT it could be Linux
  6: MKATransport transport = MKATransport.THRIFT_TRANSPORT;
  // macsecDesired field is  exchanged with peer
  7: bool macsecDesired = true;
  // For future
  8: MACSecCapability capability = CAPABILITY_NOT_IMPLEMENTED;
  // We support confidentiality offset of 0 only
  9: mka_structs.MKAConfidentialityOffset confidentOffset = mka_structs.MKAConfidentialityOffset.CONFIDENTIALITY_NO_OFFSET;
  // Timers associated with SAK
  10: MKATimers timers;
  // We will support AES_XPN_128 and AES_XPN_256
  11: mka_structs.MKACipherSuite cipher = mka_structs.MKACipherSuite.GCM_AES_XPN_128;
  // When Macsec is enabled, we'll drop unenrypted traffic
  12: bool dropUnencrypted = true;
  // Key selector to choose primary or secondary key
  13: KeyType keySelect = KeyType.PRIMARY_KEY;
  // Traffic policy, Active SAK is the default/existing behavior
  14: TrafficPolicy trafficPolicy = TrafficPolicy.POLICY_ACTIVE_SAK;
}

/*
 * MKAServiceConfig is the main structure used for generating the mka.conf
 * which resides in /etc/coop/macsec/mka.conf.
 * 1. The process like EBBGen will use this template along with router/circuit
 *    information from FBNet and generate the Switch MKA config template.
 * 2. The coop process will take that switch MKA config template and generate
 *    the Switch MKA config for that switch.
 * 3. That final config will be deserialized by FBOSS mka_service process and
 *    config load will happen which will establish Macsec sessions.
 */
struct MKAServiceConfig {
  // This is used to override the default command line arguments we
  // pass to MKA service.
  1: map<string, string> defaultCommandLineArgs = {};

  // MKA server priority will be used to resolve election among peers
  2: i16 serverPriority = DEFAULT_KEYSERVER_PRIORITY;

  // This has a list of MKA CAK keys, one per port.
  3: list<MKAConfig> portMKAConfig = [];
}
