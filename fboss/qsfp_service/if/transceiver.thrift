namespace cpp2 facebook.fboss
namespace go neteng.fboss.transceiver
namespace php fboss
namespace py neteng.fboss.transceiver
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.transceiver

include "fboss/lib/phy/link.thrift"
include "fboss/lib/phy/prbs.thrift"
include "thrift/annotation/cpp.thrift"

/*
 * UNINITIALIZED - when transceiverManager has just been created and init() is not yet called
 * INITIALIZED - when transceiverManager->init() is complete i.e. systemContainer, pimContainer, phys are initialized
 * ACTIVE - one run of refreshStateMachines is complete. Transceivers have been detected
 * EXITING - when transceiverManager is going through a graceful exit
 */
enum QsfpServiceRunState {
  UNINITIALIZED = 0,
  INITIALIZED = 1,
  ACTIVE = 2,
  EXITING = 3,
  UPGRADING_FIRMWARE = 4,
}

struct Vendor {
  1: string name;
  2: binary oui;
  3: string partNumber;
  4: string rev;
  5: string serialNumber;
  6: string dateCode;
}

struct Thresholds {
  1: double low;
  2: double high;
}

struct ThresholdLevels {
  1: Thresholds alarm;
  2: Thresholds warn;
}

struct AlarmThreshold {
  1: ThresholdLevels temp;
  2: ThresholdLevels vcc;
  3: ThresholdLevels rxPwr;
  4: ThresholdLevels txBias;
  5: ThresholdLevels txPwr;
}

struct Flags {
  1: bool high;
  2: bool low;
}

struct FlagLevels {
  1: Flags alarm;
  2: Flags warn;
}

struct Sensor {
  1: double value;
  2: optional FlagLevels flags;
}

struct GlobalSensors {
  1: Sensor temp;
  2: Sensor vcc;
}

/*
 * Sensor data for individual channels of a module. Pl note that Rx/Tx power
 * values are the real values in mW read from optics whereas the dBm values
 * are converted value from mW. Generally dBm = 10log(mW) and in case mW is
 * less than 0.01mW then we use dBm as -40
 */
struct ChannelSensors {
  1: Sensor rxPwr;
  2: Sensor txBias;
  3: Sensor txPwr;
  4: optional Sensor txSnr;
  5: optional Sensor rxSnr;
  6: optional Sensor rxPwrdBm;
  7: optional Sensor txPwrdBm;
}

struct SignalFlags {
  1: i32 txLos;
  2: i32 rxLos;
  3: i32 txLol;
  4: i32 rxLol;
}

/*
 * Currently, only support HARD_RESET
 */
enum ResetType {
  INVALID = 0,
  HARD_RESET = 1,
// SOFT_RESET = 2,
// DATAPATH_RESET = 3,
// VCC_RESET = 4,
}

/*
 * Transceiver Reset action. RESET_THEN_CLEAR resets then clears reset,
 * RESET holds the reset on the transceiver, CLEAR_RESET clears reset.
 */
enum ResetAction {
  INVALID = 0,
  RESET_THEN_CLEAR = 1,
  RESET = 2,
  CLEAR_RESET = 3,
}

enum TransmitterTechnology {
  UNKNOWN = 0,
  COPPER = 1,
  OPTICAL = 2,
  BACKPLANE = 3,
}
/*
 * QSFP and SFP units specify length as a byte;  a value of 255 indicates
 * that the cable is longer than can be represented.  Each has a different
 * scaling factor for different lengths.  We represent "longer than
 * can be represented" with a negative integer value of the appropriate
 * "max length" size -- i.e. -255000 for more than 255 km on a single
 * mode fiber.
 */
struct Cable {
  1: optional i32 singleModeKm;
  2: optional i32 singleMode;
  3: optional i32 om3;
  4: optional i32 om2;
  5: optional i32 om1;
  6: optional i32 copper;
  7: TransmitterTechnology transmitterTech;
  8: optional double length;
  9: optional i32 gauge;
  10: optional i32 om4;
  11: optional i32 om5;
}

struct Channel {
  1: i32 channel;
  6: ChannelSensors sensors;
}

enum TransceiverType {
  SFP = 0,
  QSFP = 1,
}

enum TransceiverManagementInterface {
  SFF = 0,
  CMIS = 1,
  NONE = 2,
  SFF8472 = 3,
  UNKNOWN = 4,
}

enum FeatureState {
  UNSUPPORTED = 0,
  ENABLED = 1,
  DISABLED = 2,
}

enum PowerControlState {
  POWER_LPMODE = 0,
  POWER_OVERRIDE = 1,
  POWER_SET = 2, // Deprecated!
  HIGH_POWER_OVERRIDE = 3,
  POWER_SET_BY_HW = 4,
}

enum RateSelectState {
  UNSUPPORTED = 0,
  // Depending on which of the rate selects are set, the Extended Rate
  // Compliance bits are read differently:
  // ftp://ftp.seagate.com/sff/SFF-8636.PDF page 36
  EXTENDED_RATE_SELECT_V1 = 1,
  EXTENDED_RATE_SELECT_V2 = 2,
  APPLICATION_RATE_SELECT = 3,
}

enum RateSelectSetting {
  LESS_THAN_2_2GB = 0,
  FROM_2_2GB_TO_6_6GB = 1,
  FROM_6_6GB_AND_ABOVE = 2,
  LESS_THAN_12GB = 3,
  FROM_12GB_TO_24GB = 4,
  FROM_24GB_to_26GB = 5,
  FROM_26GB_AND_ABOVE = 6,
  UNSUPPORTED = 7,
  UNKNOWN = 8,
}

// A generic media interface code enum to represent all possible media interface
// codes regardless of management interface types
enum MediaInterfaceCode {
  UNKNOWN = 0,
  CWDM4_100G = 1,
  CR4_100G = 2,
  FR1_100G = 3,
  FR4_200G = 4,
  FR4_400G = 5,
  LR4_400G_10KM = 6,
  LR_10G = 7,
  SR_10G = 8,
  CR4_200G = 9,
  CR8_400G = 10,
  FR4_2x400G = 11,
  BASE_T_10G = 12,
  DR4_2x400G = 13,
  DR4_400G = 14,
  FR8_800G = 15,
}

// The extended specification compliance code of the transceiver module.
// This is the field of Byte 192 on page00 and following table 4-4
// Extended Specification Compliance Codes of SFF-8024.
// TODO(joseph5wu) Will deprecate this enum and start using MediaInterfaceCode
enum ExtendedSpecComplianceCode {
  UNKNOWN = 0,
  CWDM4_100G = 6,
  CR4_100G = 11,
  FR1_100G = 38,
  // 50GBASE-CR, 100GBASE-CR2, or 200GBASE-CR4
  CR_50G_CHANNELS = 64,
  BASE_T_10G = 0x1C,
}

// Transceiver identifier as read from module page 0 reg 0
enum TransceiverModuleIdentifier {
  UNKNOWN = 0,
  SFP_PLUS = 0x3,
  QSFP = 0xC,
  QSFP_PLUS = 0xD,
  QSFP28 = 0x11,
  QSFP_DD = 0x18,
  QSFP_PLUS_CMIS = 0x1E,
  OSFP = 0x19,
  MINIPHOTON_OBO = 0x91,
}

enum CmisModuleState {
  UNKNOWN = 0x0,
  LOW_POWER = 0x1,
  POWERING_UP = 0x2,
  READY = 0x3,
  POWERING_DOWN = 0x4,
  FAULT = 0x5,
}

enum TransceiverFeature {
  NONE = 0,
  VDM = 0x0001,
  CDB = 0x0002,
  PRBS = 0x0004,
  LOOPBACK = 0x0008,
  TX_DISABLE = 0x0010,
}

// TODO(joseph5wu) Will deprecate this enum and start using MediaInterfaceCode
enum SMFMediaInterfaceCode {
  UNKNOWN = 0x0,
  CWDM4_100G = 0x10,
  FR1_100G = 0x15,
  FR4_200G = 0x18,
  DR4_400G = 0x1C,
  FR4_400G = 0x1D,
  LR4_10_400G = 0x1E,
  FR8_800G = 0xC1,
}

enum Ethernet10GComplianceCode {
  SR_10G = 0x10,
  LR_10G = 0x20,
  LRM_10G = 0x40,
  ER_10G = 0x80,
}

enum PassiveCuMediaInterfaceCode {
  UNKNOWN = 0x0,
  COPPER = 0x1,
  COPPER_400G = 0x3,
  LOOPBACK = 0xBF,
}

union MediaInterfaceUnion {
  1: SMFMediaInterfaceCode smfCode;
  2: ExtendedSpecComplianceCode extendedSpecificationComplianceCode;
  3: Ethernet10GComplianceCode ethernet10GComplianceCode;
  4: PassiveCuMediaInterfaceCode passiveCuCode;
}

enum MediaTypeEncodings {
  UNDEFINED = 0x0,
  OPTICAL_MMF = 0x1,
  OPTICAL_SMF = 0x2,
  PASSIVE_CU = 0x3,
  ACTIVE_CABLES = 0x4,
}

struct MediaInterfaceId {
  1: i32 lane;
  // TODO(joseph5wu) Deprecate this union attribute and use the enum `code`
  2: MediaInterfaceUnion media;
  3: MediaInterfaceCode code = MediaInterfaceCode.UNKNOWN;
}

enum CmisLaneState {
  DEACTIVATED = 0x1,
  DATAPATHINIT = 0x2,
  DEINIT = 0x3,
  ACTIVATED = 0x4,
  TX_ON = 0x5,
  TX_OFF = 0x6,
  DATAPATH_INITIALIZED = 0x7,
}

struct FirmwareStatus {
  1: optional string version;
  2: optional i32 fwFault;
  3: optional string dspFwVer;
  4: optional string buildRev;
}

struct MediaLaneSettings {
  1: i32 lane;
  2: optional bool txDisable;
  3: optional bool txSquelch;
  4: optional bool txAdaptiveEqControl; // Only applicable for Sff
  5: optional bool txSquelchForce; // Only applicable for Cmis
}

struct HostLaneSettings {
  1: i32 lane;
  2: optional i32 txInputEqualization;
  3: optional i32 rxOutputEmphasis;
  4: optional i32 rxOutputAmplitude;
  5: optional bool rxOutput;
  6: optional bool rxSquelch;
  7: optional i32 rxOutputPreCursor;
  8: optional i32 rxOutputPostCursor;
}

struct MediaLaneSignals {
  1: i32 lane;
  2: optional bool txLos; // DEPRECATED - Use HostLaneSignals instead
  3: optional bool rxLos;
  4: optional bool txLol; // DEPRECATED - Use HostLaneSignals instead
  5: optional bool rxLol;
  6: optional bool txFault;
  7: optional bool txAdaptEqFault; // DEPRECATED - Use HostLaneSignals instead
}

struct HostLaneSignals {
  1: i32 lane;
  2: optional bool dataPathDeInit;
  3: optional CmisLaneState cmisLaneState;
  4: optional bool txLos;
  5: optional bool txLol;
  6: optional bool txAdaptEqFault;
}

struct RxEqualizerSettings {
  1: i32 preCursor;
  2: i32 postCursor;
  3: i32 mainAmplitude;
}

struct VdmPerfMonitorPortSideStats {
  1: link.LinkPerfMonitorParamEachSideVal datapathBER;
  2: link.LinkPerfMonitorParamEachSideVal datapathErroredFrames;
  3: map<i32, double> laneSNR;
  4: map<i32, double> lanePam4Level0SD;
  5: map<i32, double> lanePam4Level1SD;
  6: map<i32, double> lanePam4Level2SD;
  7: map<i32, double> lanePam4Level3SD;
  8: map<i32, double> lanePam4MPI;
  9: map<i32, double> lanePam4LTP;
  10: optional i16 fecTailMax;
  11: optional i16 fecTailCurr;
}

struct VdmPerfMonitorStats {
  // Map of SW Port to Media side VDM Performance Monitor diags stats
  1: map<string, VdmPerfMonitorPortSideStats> mediaPortVdmStats;
  // Map of SW Port to Host side VDM Performance Monitor diags stats
  2: map<string, VdmPerfMonitorPortSideStats> hostPortVdmStats;
  3: i64 statsCollectionTme;
  4: i64 intervalStartTime;
}

struct VdmPerfMonitorPortSideStatsForOds {
  1: double datapathBERMax;
  2: double datapathErroredFramesMax;
  3: double laneSNRMin;
  4: double lanePam4Level0SDMax;
  5: double lanePam4Level1SDMax;
  6: double lanePam4Level2SDMax;
  7: double lanePam4Level3SDMax;
  8: double lanePam4MPIMax;
  9: double lanePam4LTPMax;
  10: optional i16 fecTailMax;
}

struct VdmPerfMonitorStatsForOds {
  // Map of SW Port to Media side VDM Performance Monitor diags stats
  1: map<string, VdmPerfMonitorPortSideStatsForOds> mediaPortVdmStats;
  // Map of SW Port to Host side VDM Performance Monitor diags stats
  2: map<string, VdmPerfMonitorPortSideStatsForOds> hostPortVdmStats;
  3: i64 statsCollectionTme;
}

struct VdmDiagsStats {
  1: double preFecBerMediaMin;
  2: double preFecBerMediaMax;
  3: double preFecBerMediaAvg;
  4: double preFecBerMediaCur;
  5: map<i32, double> eSnrMediaChannel;
  6: double preFecBerHostMin;
  7: double preFecBerHostMax;
  8: double preFecBerHostAvg;
  9: double preFecBerHostCur;
  10: i64 statsCollectionTme;
  11: double errFrameMediaMin;
  12: double errFrameMediaMax;
  13: double errFrameMediaAvg;
  14: double errFrameMediaCur;
  15: double errFrameHostMin;
  16: double errFrameHostMax;
  17: double errFrameHostAvg;
  18: double errFrameHostCur;
  19: map<i32, double> pam4Level0SDLine;
  20: map<i32, double> pam4Level1SDLine;
  21: map<i32, double> pam4Level2SDLine;
  22: map<i32, double> pam4Level3SDLine;
  23: map<i32, double> pam4MPILine;
  24: map<i32, double> pam4LtpMediaChannel;
  25: optional i16 fecTailMediaMax;
  26: optional i16 fecTailMediaCurr;
  27: optional i16 fecTailHostMax;
  28: optional i16 fecTailHostCurr;
}

struct SymErrHistogramBin {
  1: double nbitSymbolErrorMax;
  2: double nbitSymbolErrorAvg;
  3: double nbitSymbolErrorCur;
}

struct CdbDatapathSymErrHistogram {
  1: map<i32, SymErrHistogramBin> media;
  2: map<i32, SymErrHistogramBin> host;
}

struct TransceiverSettings {
  1: FeatureState cdrTx;
  2: FeatureState cdrRx;
  3: RateSelectState rateSelect;
  4: FeatureState powerMeasurement;
  5: PowerControlState powerControl;
  6: RateSelectSetting rateSelectSetting;
  7: optional list<MediaLaneSettings> mediaLaneSettings;
  8: optional list<HostLaneSettings> hostLaneSettings;
  9: optional list<MediaInterfaceId> mediaInterface;
}

// maintained and populated by qsfp service
struct TransceiverStats {
  // duration between last read and last successful read
  1: double readDownTime;
  // duration between last write and last successful write
  2: double writeDownTime;
  // Number of times the read transaction was attempted
  3: i64 numReadAttempted;
  // Number of times the read transaction failed
  4: i64 numReadFailed;
  // Number of times the write transaction was attempted
  5: i64 numWriteAttempted;
  // Number of times the write transaction failed
  6: i64 numWriteFailed;
}

struct ModuleStatus {
  1: optional bool dataNotReady;
  2: optional bool interruptL;
  3: optional CmisModuleState cmisModuleState;
  4: optional FirmwareStatus fwStatus;
  5: optional bool cmisStateChanged;
}

struct TcvrState {
  1: bool present;
  2: TransceiverType transceiver;
  3: i32 port; // physical port number
  5: optional AlarmThreshold thresholds;
  6: optional Vendor vendor;
  7: optional Cable cable;
  9: optional TransceiverSettings settings;
  10: optional SignalFlags signalFlag;
  11: optional ExtendedSpecComplianceCode extendedSpecificationComplianceCode;
  12: optional TransceiverManagementInterface transceiverManagementInterface;
  13: optional TransceiverModuleIdentifier identifier;
  14: optional ModuleStatus status;
  15: optional list<MediaLaneSignals> mediaLaneSignals;
  16: optional list<HostLaneSignals> hostLaneSignals;
  18: bool eepromCsumValid;
  19: optional MediaInterfaceCode moduleMediaInterface;
  20: optional TransceiverStateMachineState stateMachineState;
  21: map<string, list<i32>> portNameToHostLanes;
  22: map<string, list<i32>> portNameToMediaLanes;
  23: i64 timeCollected;
  24: DiagsCapability diagCapability;
  25: bool fwUpgradeInProgress;
}

struct TcvrStats {
  1: optional GlobalSensors sensor;
  2: list<Channel> channels;
  3: optional TransceiverStats stats;
  4: optional i64 remediationCounter;
  5: optional VdmDiagsStats vdmDiagsStats;
  6: optional VdmDiagsStats vdmDiagsStatsForOds;
  7: map<string, list<i32>> portNameToHostLanes;
  8: map<string, list<i32>> portNameToMediaLanes;
  9: i64 timeCollected;
  10: i64 lastFwUpgradeStartTime;
  11: i64 lastFwUpgradeEndTime;
  12: optional VdmPerfMonitorStats vdmPerfMonitorStats;
  13: optional VdmPerfMonitorStatsForOds vdmPerfMonitorStatsForOds;
  14: map<string, CdbDatapathSymErrHistogram> cdbDatapathSymErrHistogram;
  15: map<string, i64> lastDatapathResetTime;
}

struct TransceiverInfo {
  1: optional bool present (deprecated = "Moved to state/stats");
  2: optional TransceiverType transceiver (deprecated = "Moved to state/stats");
  3: optional i32 port (deprecated = "Moved to state/stats"); // physical port number
  4: optional GlobalSensors sensor (deprecated = "Moved to state/stats");
  5: optional AlarmThreshold thresholds (deprecated = "Moved to state/stats");
  9: optional Vendor vendor (deprecated = "Moved to state/stats");
  10: optional Cable cable (deprecated = "Moved to state/stats");
  12: optional list<Channel> channels (deprecated = "Moved to state/stats");
  13: optional TransceiverSettings settings (
    deprecated = "Moved to state/stats",
  );
  14: optional TransceiverStats stats (deprecated = "Moved to state/stats");
  15: optional SignalFlags signalFlag (deprecated = "Moved to state/stats");
  16: optional ExtendedSpecComplianceCode extendedSpecificationComplianceCode (
    deprecated = "Moved to state/stats",
  );
  17: optional TransceiverManagementInterface transceiverManagementInterface (
    deprecated = "Moved to state/stats",
  );
  18: optional TransceiverModuleIdentifier identifier (
    deprecated = "Moved to state/stats",
  );
  19: optional ModuleStatus status (deprecated = "Moved to state/stats");
  20: optional list<MediaLaneSignals> mediaLaneSignals (
    deprecated = "Moved to state/stats",
  );
  21: optional list<HostLaneSignals> hostLaneSignals (
    deprecated = "Moved to state/stats",
  );
  22: optional i64 timeCollected (deprecated = "Moved to state/stats");
  23: optional i64 remediationCounter (deprecated = "Moved to state/stats");
  24: optional VdmDiagsStats vdmDiagsStats (
    deprecated = "Moved to state/stats",
  );
  25: bool eepromCsumValid (deprecated = "Moved to state/stats");
  26: optional MediaInterfaceCode moduleMediaInterface (
    deprecated = "Moved to state/stats",
  );
  27: optional TransceiverStateMachineState stateMachineState (
    deprecated = "Moved to state/stats",
  );
  28: optional VdmDiagsStats vdmDiagsStatsForOds (
    deprecated = "Moved to state/stats",
  );
  // During the transition, the new state and states will be optional.
  // Both new and old fields will be filled in by QSFP service. Users
  // should checked the new fields and use it if available but fall back
  // to old fields if it's not. Once all users can understand the new
  // fields, we can then remove the old fields and make the new fields
  // non-optional. If making changes during this transition, please
  // make sure to change both the new and the old structs.
  29: TcvrState tcvrState;
  30: TcvrStats tcvrStats;
}

@cpp.Type{name = "folly::IOBuf"}
typedef binary IOBuf

struct RawDOMData {
  // The SFF DOM exposes at most 256 bytes at a time and is divided in
  // to two 128 byte "pages". The lower page is always the same, but
  // you can swap in different pages for the upper 128 bytes. The only
  // ones we use now are upper pages 0 and 3. Page 0 is required of
  // every transceiver, but the rest are optional. If we need other
  // fields in the future we can add support for other pages.
  // Added page10 and page11 for cmis optics
  1: IOBuf lower;
  2: IOBuf page0;
  3: optional IOBuf page3;
  4: optional IOBuf page10;
  5: optional IOBuf page11;
}

// Create a union data structure where we can store SffData and
// CMISData as well. After this been fully deployed we can remove the
// old RawDOMData structure.

union DOMDataUnion {
  1: Sff8636Data sff8636;
  2: CmisData cmis;
}

struct Sff8636Data {
  // The SFF DOM exposes at most 256 bytes at a time and is divided in
  // to two 128 byte "pages". The lower page is always the same, but
  // you can swap in different pages for the upper 128 bytes. The only
  // ones we use now are upper pages 0 and 3. Page 0 is required of
  // every transceiver, but the rest are optional. If we need other
  // fields in the future we can add support for other pages.
  1: IOBuf lower;
  2: IOBuf page0;
  3: optional IOBuf page3;
  4: optional i64 timeCollected;
}

struct CmisData {
  // Similar to SFF Data format, 256 bytes are exposed at a time and is
  // also divided into two 128 byte pages. Lower page stay the same but
  // we have more upper pages this time.
  1: IOBuf lower;
  2: IOBuf page0;
  4: optional IOBuf page01;
  5: optional IOBuf page02;
  6: optional IOBuf page10;
  7: optional IOBuf page11;
  8: optional IOBuf page13;
  9: optional IOBuf page14;
  10: optional i64 timeCollected;
  11: optional IOBuf page20;
  12: optional IOBuf page21;
  13: optional IOBuf page24;
  14: optional IOBuf page25;
  15: optional IOBuf page22;
  16: optional IOBuf page26;
}

struct TransceiverIOParameters {
  1: i32 offset; // should range from 0 - 255
  2: optional i32 page; // can be used to access bytes 128-255 from a different page than page0
  3: optional i32 length; // Number of bytes to read. Not applicable for a write
}

struct ReadRequest {
  1: list<i32> ids;
  2: TransceiverIOParameters parameter;
}

struct ReadResponse {
  1: bool valid;
  2: IOBuf data;
}

struct WriteRequest {
  1: list<i32> ids;
  2: TransceiverIOParameters parameter;
  3: byte data; // The data to write for a write request
}

struct WriteResponse {
  1: bool success;
}

struct DiagsCapability {
  1: bool diagnostics = false;
  2: bool vdm = false;
  3: bool cdb = false;
  4: bool prbsLine = false;
  5: bool prbsSystem = false;
  6: bool loopbackLine = false;
  7: bool loopbackSystem = false;
  8: list<prbs.PrbsPolynomial> prbsSystemCapabilities = [];
  9: list<prbs.PrbsPolynomial> prbsLineCapabilities = [];
  10: bool txOutputControl = false;
  11: bool rxOutputControl = false;
  12: bool snrLine = false;
  13: bool snrSystem = false;
  14: bool cdbFirmwareUpgrade = false;
  15: bool cdbFirmwareReadback = false;
  16: bool cdbEplMemorySupported = false;
  17: bool cdbSymbolErrorHistogramLine = false;
  18: bool cdbSymbolErrorHistogramSystem = false;
  19: bool cdbRxErrorHistogramLine = false;
  20: bool cdbRxErrorHistogramSystem = false;
}

enum TransceiverStateMachineState {
  NOT_PRESENT = 0,
  PRESENT = 1,
  DISCOVERED = 2,
  IPHY_PORTS_PROGRAMMED = 3,
  XPHY_PORTS_PROGRAMMED = 4,
  TRANSCEIVER_PROGRAMMED = 5,
  ACTIVE = 6,
  INACTIVE = 7,
  UPGRADING = 8,
  TRANSCEIVER_READY = 9,
}

struct SwitchDeploymentInfo {
  1: string dataCenter;
  2: string hostnameScheme;
}
struct TransceiverThermalData {
  1: string moduleMediaInterface;
  2: i32 temperature;
}

struct QsfpToBmcSyncData {
  1: string syncDataStructVersion;
  2: i64 timestamp;
  3: SwitchDeploymentInfo switchDeploymentInfo;
  4: map<string, TransceiverThermalData> transceiverThermalData;
}

struct FirmwareUpgradeData {
  1: string partNumber;
  2: string currentFirmwareVersion;
  3: string desiredFirmwareVersion;
}
