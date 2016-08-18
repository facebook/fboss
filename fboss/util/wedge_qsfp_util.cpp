// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/lib/usb/WedgeI2CBus.h"
#include "fboss/lib/usb/Wedge100I2CBus.h"
#include "fboss/lib/usb/UsbError.h"

#include <folly/Conv.h>
#include <folly/Memory.h>
#include <folly/FileUtil.h>
#include <folly/Exception.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sysexits.h>

#include <vector>

using namespace facebook::fboss;
using folly::MutableByteRange;
using folly::StringPiece;

// We can check on the hardware type:

const char *chipCheckPath = "/sys/bus/pci/devices/0000:01:00.0/device";
const char *trident2 = "0xb850\n";  // Note expected carriage return

DEFINE_bool(clear_low_power, false,
            "Allow the QSFP to use higher power; needed for LR4 optics");
DEFINE_bool(tx_disable, false, "Set the TX disable bits");
DEFINE_bool(tx_enable, false, "Clear the TX disable bits");
DEFINE_bool(set_40g, false, "Rate select 40G");
DEFINE_bool(set_100g, false, "Rate select 100G");
DEFINE_bool(cdr_enable, false, "Set the CDR bits if transceiver supports it");
DEFINE_bool(cdr_disable, false,
    "Clear the CDR bits if transceiver supports it");

bool overrideLowPower(TransceiverI2CApi* bus, unsigned int port) {
  // 0x01 overrides low power mode
  // 0x04 is an LR4-specific bit that is otherwise reserved
  uint8_t buf[1] = {5};
  try {
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 93, 1, buf);
  } catch (const UsbError& ex) {
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
    bus->moduleRead(port, TransceiverI2CApi::ADDR_QSFP, 129,
                      1, supported);
  } catch (const UsbError& ex) {
    fprintf(stderr, "Port %d: Unable to determine whether CDR supported\n",
        port);
    return false;
  }
  // If 2nd and 3rd bits are set, CDR is supported
  if (!(supported[0] & 0xC)) {
    fprintf(stderr, "CDR unsupported by this device, doing nothing");
    return false;
  }

  // Even if CDR isn't supported for one of RX and TX, set the whole
  // byte anyway
  uint8_t buf[1] = {value};
  try {
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 98, 1, buf);
  } catch (const UsbError& ex) {
    fprintf(stderr, "QSFP %d: Failed to set CDR\n", port);
    return false;
  }
  return true;
}

bool rateSelect(TransceiverI2CApi* bus, unsigned int port, uint8_t value) {
  // 0b11 - 25G channels
  // 0b10 - 10G channels
  uint8_t buf[1] = {value};
  try {
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 87, 1, buf);
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 88, 1, buf);
  } catch (const UsbError& ex) {
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
  } catch (const UsbError& ex) {
    fprintf(stderr, "QSFP %d: not present or unwritable\n", port);
    return false;
  }
  return true;
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

void printChannelMonitor(unsigned int index,
                         const uint8_t* buf,
                         unsigned int rxMSB,
                         unsigned int rxLSB,
                         unsigned int txMSB,
                         unsigned int txLSB) {
  uint16_t rxValue = (buf[rxMSB] << 8) | buf[rxLSB];
  uint16_t txValue = (buf[txMSB] << 8) | buf[txLSB];

  // RX power ranges from 0mW to 6.5535mW
  double rxPower = 0.0001 * rxValue;

  // TX bias ranges from 0mA to 131mA
  double txBias = (131.0 * txValue) / 65535;

  printf("    Channel %d:   %12fmW  %12fmA\n", index, rxPower, txBias);
}

void printPortDetail(TransceiverI2CApi* bus, unsigned int port) {
  uint8_t buf[256];
  try {
    // Read the lower 128 bytes.
    bus->moduleRead(port, TransceiverI2CApi::ADDR_QSFP, 0, 128, buf);

    // Read page 0 from the upper 128 bytes.
    // First see if we need to select page 0.
    if ((buf[2] & (1 << 2)) == 0) {
      uint8_t page0 = 0;
      if (buf[127] != page0) {
        bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP,
                         127, 1, &page0);
      }
    }
    bus->moduleRead(port, TransceiverI2CApi::ADDR_QSFP, 128,
                    128, buf + 128);
  } catch (const UsbError& ex) {
    // This generally means the QSFP module is not present.
    fprintf(stderr, "Port %d: not present\n", port);
    return;
  }

  printf("Port %d\n", port);
  printf("  ID: %#04x\n", buf[0]);
  printf("  Status: 0x%02x 0x%02x\n", buf[1], buf[2]);

  printf("  Interrupt Flags:\n");
  printf("    LOS: 0x%02x\n", buf[3]);
  printf("    Fault: 0x%02x\n", buf[4]);
  printf("    Temp: 0x%02x\n", buf[6]);
  printf("    Vcc: 0x%02x\n", buf[7]);
  printf("    Rx Power: 0x%02x 0x%02x\n", buf[9], buf[10]);
  printf("    Tx Bias: 0x%02x 0x%02x\n", buf[11], buf[12]);
  printf("    Reserved Set 3: 0x%02x 0x%02x\n", buf[13], buf[14]);
  printf("    Reserved Set 4: 0x%02x 0x%02x\n", buf[15], buf[16]);
  printf("    Reserved Set 5: 0x%02x 0x%02x\n", buf[17], buf[18]);
  printf("    Vendor Defined: 0x%02x 0x%02x 0x%02x\n",
         buf[19], buf[20], buf[21]);

  auto temp = static_cast<int8_t>(buf[22]) + (buf[23] / 256.0);
  printf("  Temperature: %f C\n", temp);
  uint16_t voltage = (buf[26] << 8) | buf[27];
  printf("  Supply Voltage: %f V\n", voltage / 10000.0);

  printf("  Channel Data:  %12s    %12s\n", "RX Power", "TX Bias");
  printChannelMonitor(1, buf, 34, 35, 42, 43);
  printChannelMonitor(2, buf, 36, 37, 44, 45);
  printChannelMonitor(3, buf, 38, 39, 46, 47);
  printChannelMonitor(4, buf, 40, 41, 48, 49);
  printf("    Power measurement is %s\n",
         (buf[220] & 0x04) ? "supported" : "unsupported");
  printf("    Reported RX Power is %s\n",
         (buf[220] & 0x08) ? "average power" : "OMA");

  printf("  Power set:  0x%02x\tExtended ID:  0x%02x\t"
         "Ethernet Compliance:  0x%02x\n",
         buf[93], buf[129], buf[131]);
  printf("  TX disable bits: 0x%02x\n", buf[86]);
  printf("  Rate select is %s\n",
      (buf[221] & 0x0c) ? "supported" : "unsupported");
  printf("  RX rate select bits: 0x%02x\n", buf[87]);
  printf("  TX rate select bits: 0x%02x\n", buf[88]);
  printf("  CDR support:  TX: %s\tRX: %s\n",
      (buf[129] & (1 << 3)) ? "supported" : "unsupported",
      (buf[129] & (1 << 2)) ? "supported" : "unsupported");
  printf("  CDR bits: 0x%02x\n", buf[98]);

  auto vendor = sfpString(buf, 148, 16);
  auto vendorPN = sfpString(buf, 168, 16);
  auto vendorRev = sfpString(buf, 184, 2);
  auto vendorSN = sfpString(buf, 196, 16);
  auto vendorDate = sfpString(buf, 212, 8);

  printf("  Connector: 0x%02x\n", buf[130]);
  printf("  Spec compliance: "
         "0x%02x 0x%02x 0x%02x 0x%02x"
         "0x%02x 0x%02x 0x%02x 0x%02x\n",
         buf[131], buf[132], buf[133], buf[134],
         buf[135], buf[136], buf[137], buf[138]);
  printf("  Encoding: 0x%02x\n", buf[139]);
  printf("  Nominal Bit Rate: %d MBps\n", buf[140] * 100);
  printf("  Ext rate select compliance: 0x%02x\n", buf[141]);
  printf("  Length (SMF): %d km\n", buf[142]);
  printf("  Length (OM3): %d m\n", buf[143] * 2);
  printf("  Length (OM2): %d m\n", buf[144]);
  printf("  Length (OM1): %d m\n", buf[145]);
  printf("  Length (Copper): %d m\n", buf[146]);
  printf("  Device Tech: 0x%02x\n", buf[147]);
  printf("  Ext Module: 0x%02x\n", buf[164]);
  printf("  Wavelength tolerance: 0x%02x 0x%02x\n", buf[188], buf[189]);
  printf("  Max case temp: %dC\n", buf[190]);
  printf("  CC_BASE: 0x%02x\n", buf[191]);
  printf("  Options: 0x%02x 0x%02x 0x%02x 0x%02x\n",
         buf[192], buf[193], buf[194], buf[195]);
  printf("  DOM Type: 0x%02x\n", buf[220]);
  printf("  Enhanced Options: 0x%02x\n", buf[221]);
  printf("  Reserved: 0x%02x\n", buf[222]);
  printf("  CC_EXT: 0x%02x\n", buf[223]);
  printf("  Vendor Specific:\n");
  printf("    %02x %02x %02x %02x %02x %02x %02x %02x"
         "  %02x %02x %02x %02x %02x %02x %02x %02x\n",
         buf[224], buf[225], buf[226], buf[227],
         buf[228], buf[229], buf[230], buf[231],
         buf[232], buf[233], buf[234], buf[235],
         buf[236], buf[237], buf[238], buf[239]);
  printf("    %02x %02x %02x %02x %02x %02x %02x %02x"
         "  %02x %02x %02x %02x %02x %02x %02x %02x\n",
         buf[240], buf[241], buf[242], buf[243],
         buf[244], buf[245], buf[246], buf[247],
         buf[248], buf[249], buf[250], buf[251],
         buf[252], buf[253], buf[254], buf[255]);

  printf("  Vendor: %s\n", vendor.str().c_str());
  printf("  Vendor OUI: %02x:%02x:%02x\n",
         buf[165 - 128], buf[166 - 128], buf[167 - 128]);
  printf("  Vendor PN: %s\n", vendorPN.str().c_str());
  printf("  Vendor Rev: %s\n", vendorRev.str().c_str());
  printf("  Vendor SN: %s\n", vendorSN.str().c_str());
  printf("  Date Code: %s\n", vendorDate.str().c_str());
}

bool isWedge() {
  std::string contents;
  if (!folly::readFile(chipCheckPath, contents)) {
    if (errno == ENOENT) {
      return false;
    }
    folly::throwSystemError("error reading ", chipCheckPath);
  }
  return (contents == trident2);
}

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::SetCommandLineOptionWithMode("minloglevel", "0",
                                       google::SET_FLAGS_DEFAULT);

  if (FLAGS_set_100g && FLAGS_set_40g) {
    fprintf(stderr, "Cannot set both 40g and 100g\n");
    return EX_USAGE;
  }
  if (FLAGS_cdr_enable && FLAGS_cdr_disable) {
    fprintf(stderr, "Cannot set and clear the CDR bits\n");
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

  // TODO(klahey):  Should probably verify the other chip architecture.
  std::unique_ptr<TransceiverI2CApi> bus;
  if (isWedge()) {
    bus = folly::make_unique<WedgeI2CBus>();
  } else {
    bus = folly::make_unique<Wedge100I2CBus>();
  }
  try {
    bus->open();
  } catch (const std::exception& ex) {
    fprintf(stderr, "error: unable to open device: %s\n", ex.what());
    return EX_IOERR;
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

  bool printInfo = !(FLAGS_clear_low_power || FLAGS_tx_disable ||
                     FLAGS_tx_enable || FLAGS_set_100g || FLAGS_set_40g ||
                     FLAGS_cdr_enable || FLAGS_cdr_disable);
  int retcode = EX_OK;
  for (unsigned int portNum : ports) {
    if (FLAGS_clear_low_power && overrideLowPower(bus.get(), portNum)) {
      printf("QSFP %d: cleared low power flags\n", portNum);
    }
    if (FLAGS_tx_disable && setTxDisable(bus.get(), portNum, 0x0f)) {
      printf("QSFP %d: disabled TX on all channels\n", portNum);
    }
    if (FLAGS_tx_enable && setTxDisable(bus.get(), portNum, 0x00)) {
      printf("QSFP %d: enabled TX on all channels\n", portNum);
    }

    if (FLAGS_set_40g && rateSelect(bus.get(), portNum, 0xaa)) {
      printf("QSFP %d: set to optimize for 10G channels\n", portNum);
    }
    if (FLAGS_set_100g && rateSelect(bus.get(), portNum, 0xff)) {
      printf("QSFP %d: set to optimize for 25G channels\n", portNum);
    }

    if (FLAGS_cdr_enable && setCdr(bus.get(), portNum, 0xff)) {
      printf("QSFP %d: CDR enabled\n", portNum);
    }

    if (FLAGS_cdr_disable && setCdr(bus.get(), portNum, 0x00)) {
      printf("QSFP %d: CDR disabled\n", portNum);
    }

    if (printInfo) {
      try {
        printPortDetail(bus.get(), portNum);
      } catch (const std::exception& ex) {
        fprintf(stderr, "error parsing QSFP data %u: %s\n", portNum, ex.what());
        retcode = EX_SOFTWARE;
      }
    }
  }
  return retcode;
}
