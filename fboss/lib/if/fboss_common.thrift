namespace cpp2 facebook.fboss
namespace go neteng.fboss.fboss_common
namespace php fboss_common
namespace py neteng.fboss.fboss_common
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.fboss_common

enum PlatformType {
  PLATFORM_WEDGE = 1,
  PLATFORM_WEDGE100 = 2,
  PLATFORM_GALAXY_LC = 3,
  PLATFORM_GALAXY_FC = 4,
  PLATFORM_FAKE_WEDGE = 5,
  PLATFORM_MINIPACK = 6,
  PLATFORM_YAMP = 7,
  PLATFORM_FAKE_WEDGE40 = 8,
  PLATFORM_WEDGE400C = 9,
  PLATFORM_WEDGE400C_SIM = 10,
  PLATFORM_WEDGE400 = 11,
  PLATFORM_FUJI = 12,
  PLATFORM_ELBERT = 13,
  PLATFORM_CLOUDRIPPER_DEPRECATED = 14,
  PLATFORM_DARWIN = 15,
  PLATFORM_LASSEN_DEPRECATED = 16,
  PLATFORM_SANDIA = 17,
  PLATFORM_MERU400BIU = 18,
  PLATFORM_MERU400BFU = 19,
  PLATFORM_MERU400BIA = 20,
  PLATFORM_WEDGE400C_VOQ = 21,
  PLATFORM_WEDGE400C_FABRIC = 22,
  PLATFORM_CLOUDRIPPER_VOQ_DEPRECATED = 23,
  PLATFORM_CLOUDRIPPER_FABRIC_DEPRECATED = 24,
  PLATFORM_MONTBLANC = 25,
  PLATFORM_WEDGE400C_GRANDTETON = 26,
  PLATFORM_WEDGE400_GRANDTETON = 27,
  PLATFORM_MERU800BIA = 28,
  PLATFORM_MORGAN800CC = 29,
  PLATFORM_MERU800BFA = 30,
  PLATFORM_FAKE_SAI = 31,
  PLATFORM_JANGA800BIC = 32,
  PLATFORM_TAHAN800BC = 33,
  PLATFORM_MERU800BFA_P1 = 34,
  PLATFORM_MERU800BIAB = 35,
  PLATFORM_YANGRA = 36,
  PLATFORM_DARWIN48V = 37,
  PLATFORM_MINIPACK3N = 38,
  PLATFORM_ICECUBE800BC = 39,

  # Placeholder for unknown platform type
  PLATFORM_UNKNOWN = 1000,
}

enum SdkVersionRolloutType {
  PRODUCTION = 1,
  RELEASE_CANDIDATE = 2,
}

struct PlatformAgentSdkVersion {
  1: SdkVersionRolloutType rolloutType;
  2: optional string asicSdk;
  3: optional string saiSdk;
  4: optional string saiVersion;
}

struct PlatformQsfpSdkVersion {
  1: SdkVersionRolloutType rolloutType;
  2: string version;
}
