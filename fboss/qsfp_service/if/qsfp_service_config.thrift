#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace cpp2 facebook.fboss.cfg
namespace go neteng.fboss.qsfp_service_config
namespace py neteng.fboss.qsfp_service_config
namespace py3 neteng.fboss
namespace rust facebook.fboss

struct Sff8636Overrides {
  1: optional i16 rxPreemphasis
}

struct CmisOverrides {}

union TransceiverOverrides {
  1: Sff8636Overrides sff,
  2: CmisOverrides cmis,
}

struct QsfpServiceConfig {
  // This is used to override the default command line arguments we
  // pass to qsfp service.
  1: map<string, string> defaultCommandLineArgs = {},

  // This has overrides from part number to settings on the optic.
  2: map<string, TransceiverOverrides> pnOverrides = {},
}
