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

struct MKAConfig {
  1: string l2Port;
  2: string srcMac;
  // The maximum priority value should be 255.
  // The datastructure here should be uint8_t (0-255)
  3: i16 priority = DEFAULT_KEYSERVER_PRIORITY;
  4: mka_structs.Cak primaryCak;
  5: optional mka_structs.Cak secondaryCak;
  6: MKATransport transport;
  7: bool macsecDesired = true;
  8: MACSecCapability capability = CAPABILITY_NOT_IMPLEMENTED;
  9: mka_structs.MKAConfidentialityOffset confidentOffset = mka_structs.MKAConfidentialityOffset.CONFIDENTIALITY_NO_OFFSET;
  10: MKATimers timers;
  // Arista switch supports only AES_XPN_128 and AES_XPN_256
  11: mka_structs.MKACipherSuite cipher = mka_structs.MKACipherSuite.GCM_AES_XPN_128;
}
