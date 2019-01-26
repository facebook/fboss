// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/util/wedge_qsfp_util.h"

#include "fboss/lib/usb/WedgeI2CBus.h"
#include "fboss/lib/usb/Wedge100I2CBus.h"
#include "fboss/lib/usb/GalaxyI2CBus.h"

#include "fboss/qsfp_service/lib/QsfpClient.h"

#include <folly/Conv.h>
#include <folly/Exception.h>
#include <folly/FileUtil.h>
#include <folly/Memory.h>
#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <chrono>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sysexits.h>

#include <thread>
#include <vector>
#include <utility>

using namespace facebook::fboss;
using folly::MutableByteRange;
using folly::StringPiece;
using std::pair;
using std::make_pair;
using std::chrono::seconds;
using std::chrono::steady_clock;

// We can check on the hardware type:

const char *chipCheckPath = "/sys/bus/pci/devices/0000:01:00.0/device";
const char *trident2 = "0xb850\n";  // Note expected carriage return
static constexpr uint16_t hexBase = 16;
static constexpr uint16_t decimalBase = 10;
static constexpr uint16_t eePromDefault = 255;
static constexpr uint16_t maxGauge = 30;

DEFINE_bool(clear_low_power, false,
            "Allow the QSFP to use higher power; needed for LR4 optics");
DEFINE_bool(set_low_power, false,
            "Force the QSFP to limit power usage; Only useful for testing");
DEFINE_bool(tx_disable, false, "Set the TX disable bits");
DEFINE_bool(tx_enable, false, "Clear the TX disable bits");
DEFINE_bool(set_40g, false, "Rate select 40G");
DEFINE_bool(set_100g, false, "Rate select 100G");
DEFINE_bool(cdr_enable, false, "Set the CDR bits if transceiver supports it");
DEFINE_bool(cdr_disable, false,
    "Clear the CDR bits if transceiver supports it");
DEFINE_int32(open_timeout, 30, "Number of seconds to wait to open bus");
DEFINE_bool(direct_i2c, false,
    "Read Transceiver info from i2c bus instead of qsfp_service");

bool overrideLowPower(
    TransceiverI2CApi* bus,
    unsigned int port,
    uint8_t value) {
  // 0x01 overrides low power mode
  // 0x04 is an LR4-specific bit that is otherwise reserved
  uint8_t buf[1] = {value};
  try {
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 93, 1, buf);
  } catch (const I2cError& ex) {
    // This generally means the QSFP module is not present.
    fprintf(stderr, "QSFP %d: not present or unwritable\n", port);
    return false;
  }
  return true;
}

bool setCdr(TransceiverI2CApi* bus, unsigned int port, uint8_t value) {
  // 0xff to enable
  // 0x00 to disable

  // Check if CDR is supported
  uint8_t supported[1];
  try {
    // ensure we have page0 selected
    uint8_t page0 = 0;
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP,
                     127, 1, &page0);

    bus->moduleRead(port, TransceiverI2CApi::ADDR_QSFP, 129,
                      1, supported);
  } catch (const I2cError& ex) {
    fprintf(stderr, "Port %d: Unable to determine whether CDR supported: %s\n",
            port, ex.what());
    return false;
  }
  // If 2nd and 3rd bits are set, CDR is supported.
  if ((supported[0] & 0xC) != 0xC) {
    fprintf(stderr, "CDR unsupported by this device, doing nothing");
    return false;
  }

  // Even if CDR isn't supported for one of RX and TX, set the whole
  // byte anyway
  uint8_t buf[1] = {value};
  try {
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 98, 1, buf);
  } catch (const I2cError& ex) {
    fprintf(stderr, "QSFP %d: Failed to set CDR\n", port);
    return false;
  }
  return true;
}

bool rateSelect(TransceiverI2CApi* bus, unsigned int port, uint8_t value) {
  // If v1 is used, both at 10, if v2
  // 0b10 - 25G channels
  // 0b00 - 10G channels
  uint8_t version[1];
  try {
    // ensure we have page0 selected
    uint8_t page0 = 0;
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP,
                     127, 1, &page0);

    bus->moduleRead(port, TransceiverI2CApi::ADDR_QSFP, 141,
                      1, version);
  } catch (const I2cError& ex) {
    fprintf(stderr,
        "Port %d: Unable to determine rate select version in use, defaulting \
        to V1\n",
        port);
    version[0] = 0b01;
  }

  uint8_t buf[1];
  if (version[0] & 1) {
    buf[0] = 0b10;
  } else {
    buf[0] = value;
  }

  try {
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 87, 1, buf);
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 88, 1, buf);
  } catch (const I2cError& ex) {
    // This generally means the QSFP module is not present.
    fprintf(stderr, "QSFP %d: not present or unwritable\n", port);
    return false;
  }
  return true;
}

bool setTxDisable(TransceiverI2CApi* bus, unsigned int port, uint8_t value) {
  uint8_t buf[1] = {value};
  try {
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 86, 1, buf);
  } catch (const I2cError& ex) {
    fprintf(stderr, "QSFP %d: not present or unwritable\n", port);
    return false;
  }
  return true;
}

std::map<int32_t, RawDOMData> fetchDataFromQsfpService(
    const std::vector<int32_t>& ports) {
  folly::EventBase evb;
  auto qsfpServiceClient = QsfpClient::createClient(&evb);
  auto client = std::move(qsfpServiceClient).getVia(&evb);

  std::map<int32_t, TransceiverInfo> qsfpInfoMap;

  client->sync_getTransceiverInfo(qsfpInfoMap, ports);

  std::vector<int32_t> presentPorts;
  for(auto& qsfpInfo : qsfpInfoMap) {
    if(qsfpInfo.second.present) {
      presentPorts.push_back(qsfpInfo.first);
    }
  }

  std::map<int32_t, RawDOMData> rawDOMDataMap;

  if(!presentPorts.empty()) {
    client->sync_getTransceiverRawDOMData(rawDOMDataMap, presentPorts);
  }

  return rawDOMDataMap;
}

RawDOMData fetchDataFromLocalI2CBus(TransceiverI2CApi* bus, unsigned int port) {
  RawDOMData rawDOMData;
  rawDOMData.lower = IOBuf(IOBuf::CREATE, 128);
  rawDOMData.page0 = IOBuf(IOBuf::CREATE, 128);

  // Read the lower 128 bytes.
  bus->moduleRead(port, TransceiverI2CApi::ADDR_QSFP, 0,
    128, rawDOMData.lower.writableData());

  uint8_t flatMem = rawDOMData.lower.data()[2] & (1 << 2);

  // Read page 0 from the upper 128 bytes.
  // First see if we need to select page 0.
  if (!flatMem) {
    uint8_t page0 = 0;
    if (rawDOMData.lower.data()[127] != page0) {
      bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP,
                       127, 1, &page0);
    }
  }
  bus->moduleRead(port, TransceiverI2CApi::ADDR_QSFP, 128,
    128, rawDOMData.page0.writableData());

  // Make sure page3 exist
  if (!flatMem) {
    rawDOMData.page3_ref().value_unchecked() = IOBuf(IOBuf::CREATE, 128);
    uint8_t page3 = 0x3;
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 127, 1, &page3);

    // read page 3
    bus->moduleRead(
        port,
        TransceiverI2CApi::ADDR_QSFP,
        128,
        128,
        rawDOMData.page3_ref().value_unchecked().writableData());
    rawDOMData.__isset.page3 = true;
  }
  return rawDOMData;
}

void printPortSummary(TransceiverI2CApi*) {
  // TODO: Implement code for showing a summary of all ports.
  // At the moment I haven't tested this since my test switch has some
  // 3M modules plugged in that hang up the bus sometimes when accessed by our
  // CP2112 switch.  I'll implement this in a subsequent diff.
  fprintf(stderr, "Please specify a port number\n");
  exit(1);
}

StringPiece sfpString(const uint8_t* buf, size_t offset, size_t len) {
  const uint8_t* start = buf + offset;
  while (len > 0 && start[len - 1] == ' ') {
    --len;
  }
  return StringPiece(reinterpret_cast<const char*>(start), len);
}

void printThresholds(
    const std::string& name,
    const uint8_t* data,
    std::function<double(uint16_t)> conversionCb) {
  std::array<std::array<uint8_t, 2>, 4> byteOffset{{
      {0, 1},
      {2, 3},
      {4, 5},
      {6, 7},
  }};

  printf("\n");
  const std::array<std::string, 4> thresholds{
      "High Alarm", "Low Alarm", "High Warning", "Low Warning"};

  for (auto row = 0; row < 4; row++) {
    uint16_t u16 = 0;
    for (auto col = 0; col < 2; col++) {
      u16 = (u16 << 8 | data[byteOffset[row][col]]);
    }
    printf(
        "%10s %12s %f\n",
        name.c_str(),
        thresholds[row].c_str(),
        conversionCb(u16));
  }
}

void printChannelMonitor(unsigned int index,
                         const uint8_t* buf,
                         unsigned int rxMSB,
                         unsigned int rxLSB,
                         unsigned int txBiasMSB,
                         unsigned int txBiasLSB,
                         unsigned int txPowerMSB,
                         unsigned int txPowerLSB) {
  uint16_t rxValue = (buf[rxMSB] << 8) | buf[rxLSB];
  uint16_t txPowerValue = (buf[txPowerMSB] << 8) | buf[txPowerLSB];
  uint16_t txBiasValue = (buf[txBiasMSB] << 8) | buf[txBiasLSB];

  // RX power ranges from 0mW to 6.5535mW
  double rxPower = 0.0001 * rxValue;

  // TX power ranges from 0mW to 6.5535mW
  double txPower = 0.0001 * txPowerValue;

  // TX bias ranges from 0mA to 131mA
  double txBias = (131.0 * txBiasValue) / 65535;

  printf("    Channel %d:   %12fmW  %12fmW  %12fmA\n",
         index, rxPower, txPower, txBias);
}

void printPortDetail(const RawDOMData& rawDOMData, unsigned int port) {

  auto lowerBuf = rawDOMData.lower.data();
  auto page0Buf = rawDOMData.page0.data();

  printf("Port %d\n", port);
  printf("  ID: %#04x\n", lowerBuf[0]);
  printf("  Status: 0x%02x 0x%02x\n", lowerBuf[1], lowerBuf[2]);

  printf("  Interrupt Flags:\n");
  printf("    LOS: 0x%02x\n", lowerBuf[3]);
  printf("    Fault: 0x%02x\n", lowerBuf[4]);
  printf("    Temp: 0x%02x\n", lowerBuf[6]);
  printf("    Vcc: 0x%02x\n", lowerBuf[7]);
  printf("    Rx Power: 0x%02x 0x%02x\n", lowerBuf[9], lowerBuf[10]);
  printf("    Tx Power: 0x%02x 0x%02x\n", lowerBuf[13], lowerBuf[14]);
  printf("    Tx Bias: 0x%02x 0x%02x\n", lowerBuf[11], lowerBuf[12]);
  printf("    Reserved Set 4: 0x%02x 0x%02x\n",
          lowerBuf[15], lowerBuf[16]);
  printf("    Reserved Set 5: 0x%02x 0x%02x\n",
          lowerBuf[17], lowerBuf[18]);
  printf("    Vendor Defined: 0x%02x 0x%02x 0x%02x\n",
         lowerBuf[19], lowerBuf[20], lowerBuf[21]);

  auto temp = static_cast<int8_t>
              (lowerBuf[22]) + (lowerBuf[23] / 256.0);
  printf("  Temperature: %f C\n", temp);
  uint16_t voltage = (lowerBuf[26] << 8) | lowerBuf[27];
  printf("  Supply Voltage: %f V\n", voltage / 10000.0);

  printf("  Channel Data:  %12s    %12s    %12s\n",
         "RX Power", "TX Power", "TX Bias");
  printChannelMonitor(1, lowerBuf, 34, 35, 42, 43, 50, 51);
  printChannelMonitor(2, lowerBuf, 36, 37, 44, 45, 52, 53);
  printChannelMonitor(3, lowerBuf, 38, 39, 46, 47, 54, 55);
  printChannelMonitor(4, lowerBuf, 40, 41, 48, 49, 56, 57);
  printf("    Power measurement is %s\n",
         (page0Buf[92] & 0x04) ? "supported" : "unsupported");
  printf("    Reported RX Power is %s\n",
         (page0Buf[92] & 0x08) ? "average power" : "OMA");

  printf("  Power set:  0x%02x\tExtended ID:  0x%02x\t"
         "Ethernet Compliance:  0x%02x\n",
         lowerBuf[93], page0Buf[1], page0Buf[3]);
  printf("  TX disable bits: 0x%02x\n", lowerBuf[86]);
  printf("  Rate select is %s\n",
      (page0Buf[93] & 0x0c) ? "supported" : "unsupported");
  printf("  RX rate select bits: 0x%02x\n", lowerBuf[87]);
  printf("  TX rate select bits: 0x%02x\n", lowerBuf[88]);
  printf("  CDR support:  TX: %s\tRX: %s\n",
      (page0Buf[1] & (1 << 3)) ? "supported" : "unsupported",
      (page0Buf[1] & (1 << 2)) ? "supported" : "unsupported");
  printf("  CDR bits: 0x%02x\n", lowerBuf[98]);

  auto vendor = sfpString(page0Buf, 20, 16);
  auto vendorPN = sfpString(page0Buf, 40, 16);
  auto vendorRev = sfpString(page0Buf, 56, 2);
  auto vendorSN = sfpString(page0Buf, 68, 16);
  auto vendorDate = sfpString(page0Buf, 84, 8);

  int gauge = page0Buf[109];
  auto cableGauge = gauge;
  if (gauge == eePromDefault && gauge > maxGauge) {
    // gauge implemented as hexadecimal (why?). Convert to decimal
    cableGauge = (gauge / hexBase) * decimalBase + gauge % hexBase;
  } else {
    cableGauge = 0;
  }

  printf("  Connector: 0x%02x\n", page0Buf[2]);
  printf("  Spec compliance: "
         "0x%02x 0x%02x 0x%02x 0x%02x"
         "0x%02x 0x%02x 0x%02x 0x%02x\n",
         page0Buf[3], page0Buf[4], page0Buf[5], page0Buf[6],
         page0Buf[7], page0Buf[8], page0Buf[9], page0Buf[10]);
  printf("  Encoding: 0x%02x\n", page0Buf[11]);
  printf("  Nominal Bit Rate: %d MBps\n", page0Buf[12] * 100);
  printf("  Ext rate select compliance: 0x%02x\n", page0Buf[13]);
  printf("  Length (SMF): %d km\n", page0Buf[14]);
  printf("  Length (OM3): %d m\n", page0Buf[15] * 2);
  printf("  Length (OM2): %d m\n", page0Buf[16]);
  printf("  Length (OM1): %d m\n", page0Buf[17]);
  printf("  Length (Copper): %d m\n", page0Buf[18]);
  if (page0Buf[108] != eePromDefault) {
    auto fractional = page0Buf[108] * .1;
    auto effective = fractional >= 1 ? fractional : page0Buf[18];
    printf("  Length (Copper dM): %.1f m\n", fractional);
    printf("  Length (Copper effective): %.1f m\n", effective);
  }
  if (cableGauge > 0){
    printf("  DAC Cable Gauge: %d\n", cableGauge);
  }
  printf("  Device Tech: 0x%02x\n", page0Buf[19]);
  printf("  Ext Module: 0x%02x\n", page0Buf[36]);
  printf("  Wavelength tolerance: 0x%02x 0x%02x\n",
         page0Buf[60], page0Buf[61]);
  printf("  Max case temp: %dC\n", page0Buf[62]);
  printf("  CC_BASE: 0x%02x\n", page0Buf[63]);
  printf("  Options: 0x%02x 0x%02x 0x%02x 0x%02x\n",
         page0Buf[64], page0Buf[65],
         page0Buf[66], page0Buf[67]);
  printf("  DOM Type: 0x%02x\n", page0Buf[92]);
  printf("  Enhanced Options: 0x%02x\n", page0Buf[93]);
  printf("  Reserved: 0x%02x\n", page0Buf[94]);
  printf("  CC_EXT: 0x%02x\n", page0Buf[95]);
  printf("  Vendor Specific:\n");
  printf("    %02x %02x %02x %02x %02x %02x %02x %02x"
         "  %02x %02x %02x %02x %02x %02x %02x %02x\n",
         page0Buf[96], page0Buf[97], page0Buf[98], page0Buf[99],
         page0Buf[100], page0Buf[101], page0Buf[102], page0Buf[103],
         page0Buf[104], page0Buf[105], page0Buf[106], page0Buf[107],
         page0Buf[108], page0Buf[109], page0Buf[110], page0Buf[111]);
  printf("    %02x %02x %02x %02x %02x %02x %02x %02x"
         "  %02x %02x %02x %02x %02x %02x %02x %02x\n",
         page0Buf[112], page0Buf[113], page0Buf[114], page0Buf[115],
         page0Buf[116], page0Buf[117], page0Buf[118], page0Buf[119],
         page0Buf[120], page0Buf[121], page0Buf[122], page0Buf[123],
         page0Buf[124], page0Buf[125], page0Buf[126], page0Buf[127]);

  printf("  Vendor: %s\n", vendor.str().c_str());
  printf("  Vendor OUI: %02x:%02x:%02x\n", lowerBuf[165 - 128],
          lowerBuf[166 - 128], lowerBuf[167 - 128]);
  printf("  Vendor PN: %s\n", vendorPN.str().c_str());
  printf("  Vendor Rev: %s\n", vendorRev.str().c_str());
  printf("  Vendor SN: %s\n", vendorSN.str().c_str());
  printf("  Date Code: %s\n", vendorDate.str().c_str());

  // print page3 values
  if (!rawDOMData.__isset.page3) {
    return;
  }

  auto page3Buf = rawDOMData.page3_ref().value_unchecked().data();

  printThresholds("Temp", &page3Buf[0], [](const uint16_t u16_temp) {
    double data;
    data = u16_temp / 256.0;
    if (data > 128) {
      data = data - 256;
    }
    return data;
  });

  printThresholds("Vcc", &page3Buf[16], [](const uint16_t u16_vcc) {
    double data;
    data = u16_vcc / 10000.0;
    return data;
  });

  printThresholds("Rx Power", &page3Buf[48], [](const uint16_t u16_rxpwr) {
    double data;
    data = u16_rxpwr * 0.1 / 1000;
    return data;
  });

  printThresholds("Tx Bias", &page3Buf[56], [](const uint16_t u16_txbias) {
    double data;
    data = u16_txbias * 2.0 / 1000;
    return data;
  });
}

bool isTrident2() {
  std::string contents;
  if (!folly::readFile(chipCheckPath, contents)) {
    if (errno == ENOENT) {
      return false;
    }
    folly::throwSystemError("error reading ", chipCheckPath);
  }
  return (contents == trident2);
}

void tryOpenBus(TransceiverI2CApi* bus) {
  auto expireTime = steady_clock::now() + seconds(FLAGS_open_timeout);
  while (true) {
    try {
      bus->open();
      return;
    } catch (const std::exception& ex) {
      if (steady_clock::now() > expireTime) {
        throw;
      }
    }
    usleep(100);
  }
}


int main(int argc, char* argv[]) {
  folly::init(&argc, &argv, true);
  gflags::SetCommandLineOptionWithMode(
      "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);

  if (FLAGS_set_100g && FLAGS_set_40g) {
    fprintf(stderr, "Cannot set both 40g and 100g\n");
    return EX_USAGE;
  }
  if (FLAGS_cdr_enable && FLAGS_cdr_disable) {
    fprintf(stderr, "Cannot set and clear the CDR bits\n");
    return EX_USAGE;
  }
  if (FLAGS_clear_low_power && FLAGS_set_low_power) {
    fprintf(stderr, "Cannot set and clear lp mode\n");
    return EX_USAGE;
  }

  std::vector<unsigned int> ports;
  bool good = true;
  for (int n = 1; n < argc; ++n) {
    unsigned int portNum;
    try {
      if (argv[n][0] == 'x' && argv[n][1] == 'e') {
        portNum = 1 + folly::to<unsigned int>(argv[n] + 2);
      } else {
        portNum = folly::to<unsigned int>(argv[n]);
      }
      ports.push_back(portNum);
    } catch (const std::exception& ex) {
      fprintf(stderr, "error: invalid port number \"%s\": %s\n",
              argv[n], ex.what());
      good = false;
    }
  }
  if (!good) {
    return EX_USAGE;
  }
  auto busAndError = getTransceiverAPI();
  if (busAndError.second) {
      return busAndError.second;
  }
  auto bus = std::move(busAndError.first);

  bool printInfo = !(FLAGS_clear_low_power || FLAGS_tx_disable ||
                     FLAGS_tx_enable || FLAGS_set_100g || FLAGS_set_40g ||
                     FLAGS_cdr_enable || FLAGS_cdr_disable ||
                     FLAGS_set_low_power);

  if (FLAGS_direct_i2c || !printInfo) {
    try {
      tryOpenBus(bus.get());
    } catch (const std::exception& ex) {
        fprintf(stderr, "error: unable to open device: %s\n", ex.what());
        return EX_IOERR;
    }
  } else {
    try {
      std::vector<int32_t> idx;
      for(auto port : ports) {
        // Direct I2C bus starts from 1 instead of 0, however qsfp_service index
        // starts from 0. So here we try to comply to match that behavior.
        idx.push_back(port - 1);
      }
      auto rawDOMDataMap = fetchDataFromQsfpService(idx);
      for (auto& i : idx) {
        auto iter = rawDOMDataMap.find(i);
        if(iter == rawDOMDataMap.end()) {
          fprintf(stderr, "Port %d is not present.\n", i + 1);
        }
        else {
          printPortDetail(iter->second, iter->first + 1);
        }
      }
      return EX_OK;
    } catch (const std::exception& e) {
      fprintf(stderr, "Exception talking to qsfp_service: %s\n", e.what());
      return EX_SOFTWARE;
    }
  }

  if (ports.empty()) {
    try {
      printPortSummary(bus.get());
    } catch (const std::exception& ex) {
      fprintf(stderr, "error: %s\n", ex.what());
      return EX_SOFTWARE;
    }
    return EX_OK;
  }

  int retcode = EX_OK;
  for (unsigned int portNum : ports) {
    if (FLAGS_clear_low_power && overrideLowPower(bus.get(), portNum, 0x5)) {
      printf("QSFP %d: cleared low power flags\n", portNum);
    }
    if (FLAGS_set_low_power && overrideLowPower(bus.get(), portNum, 0x0)) {
      printf("QSFP %d: set low power flags\n", portNum);
    }
    if (FLAGS_tx_disable && setTxDisable(bus.get(), portNum, 0x0f)) {
      printf("QSFP %d: disabled TX on all channels\n", portNum);
    }
    if (FLAGS_tx_enable && setTxDisable(bus.get(), portNum, 0x00)) {
      printf("QSFP %d: enabled TX on all channels\n", portNum);
    }

    if (FLAGS_set_40g && rateSelect(bus.get(), portNum, 0x0)) {
      printf("QSFP %d: set to optimize for 10G channels\n", portNum);
    }
    if (FLAGS_set_100g && rateSelect(bus.get(), portNum, 0xaa)) {
      printf("QSFP %d: set to optimize for 25G channels\n", portNum);
    }

    if (FLAGS_cdr_enable && setCdr(bus.get(), portNum, 0xff)) {
      printf("QSFP %d: CDR enabled\n", portNum);
    }

    if (FLAGS_cdr_disable && setCdr(bus.get(), portNum, 0x00)) {
      printf("QSFP %d: CDR disabled\n", portNum);
    }

    if (FLAGS_direct_i2c && printInfo) {
      try {
        printPortDetail(fetchDataFromLocalI2CBus(bus.get(), portNum), portNum);
      } catch (const I2cError& ex) {
        // This generally means the QSFP module is not present.
        fprintf(stderr, "Port %d: not present: %s\n", portNum, ex.what());
        retcode = EX_SOFTWARE;
      } catch (const std::exception& ex) {
        fprintf(stderr, "error parsing QSFP data %u: %s\n", portNum, ex.what());
        retcode = EX_SOFTWARE;
      }
    }
  }
  return retcode;
}
