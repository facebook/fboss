// Copyright (c) Meta Platforms, Inc. and affiliates.

namespace cpp2 facebook.fboss.platform.platform_manager
namespace hack NetengFbossPlatformManager
namespace py3 fboss.platform.platform_manager

// List of allowed PMUnit names that can be used in platform configurations.
// This ensures consistency across platform configs and prevents typos.
const list<string> ALLOWED_PMUNIT_NAMES = [
  // List of allowed standard PMUnit names
  "FAN_TRAY",
  "3V3_L",
  "3V3_R",
  "BCB",
  "BMC",
  "FAN",
  "FCB",
  "FCB_B",
  "FCB_T",
  "JUMPER",
  "MCB",
  "NETLAKE",
  "PDB",
  "PDB_L",
  "PDB_R",
  "PEM",
  "PIC",
  "PIC_B",
  "PIC_T",
  "PIM_16Q",
  "PIM_8DD",
  "PSU",
  "RACKMON",
  "RUNBMC",
  "SCM",
  "SMB",
  "RTM_L",
  "RTM_R",
  "SMB_L",
  "SMB_R",
  // ======= EXCEPTIONS (should not be used in new platforms) ========
  // MINIPACK3 has unique naming in eeproms
  "MINIPACK3_3V3_L",
  "MINIPACK3_3V3_R",
  "MINIPACK3_BMC",
  "MINIPACK3_FCB_B",
  "MINIPACK3_FCB_T",
  "MINIPACK3_PDB_L",
  "MINIPACK3_PDB_R",
  "MINIPACK3_SCM",
  "MINIPACK3_SMB",
  // The BIOS infers the PlatformName from MCB EEPROM in these platforms
  // This is for platforms reliant on NETLAKE BIOS.
  "MINIPACK3_MCB",
  "MINIPACK3BA_MCB",
  "MINIPACK3N_MCB",
  "ICECUBE_MCB",
  "ICETEA_MCB",
  "TAHANSB800BC_MCB",
  "LADAKH800BCLS_MCB",
  // The whole board is a PmUnit for these
  "TAHAN",
  "JANGA",
  // Misc
  "MINERVA_BMC",
  "MORGAN800CC",
  "YOLO_MAX",
  "SMB_FRU",
  "PSU_2GH",
];

// List of platforms that are allowed to have chassisEepromDevicePath
// pointing to an IDPROM device. This is a legacy exception list.
// New platforms should NOT use IDPROM for chassisEepromDevicePath.
const list<string> PLATFORMS_WITH_IDPROM_CHASSIS_EEPROM = [
  "MERU800BFA",
  "MERU800BIA",
  "MORGAN800CC",
  "JANGA800BIC",
  "TAHAN800BC",
];
