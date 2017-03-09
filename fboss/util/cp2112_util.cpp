// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/lib/usb/CP2112.h"

#include <folly/Conv.h>
#include <folly/Range.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include <map>
#include <vector>

using namespace facebook::fboss;
using folly::ByteRange;
using folly::MutableByteRange;
using folly::StringPiece;
using std::chrono::milliseconds;
using std::map;
using std::vector;

DEFINE_bool(list_commands, false,
            "Print the list of commands");
DEFINE_bool(reset, false,
            "Reset the CP2112 device upon starting");
DEFINE_bool(init_smbus_config, true,
            "Initialize the CP2112 SMBus configuration settings before "
            "performing any other operations");
DEFINE_int32(timeout_ms, -1,
            "The default I2C timeout, in milliseconds");

// Flags for the set_gpio_config command.
DEFINE_int32(gpio_direction, -1,
             "The GPIO direction bitmap for the set_gpio_config command");
DEFINE_int32(gpio_push_pull, -1,
             "The GPIO push-pull bitmap for the set_gpio_config command");
DEFINE_int32(gpio_special, -1,
             "The GPIO special bitmap for the set_gpio_config command");
DEFINE_int32(gpio_clock_divider, -1,
             "The GPIO clock divider for the set_gpio_config command");

// Flags for the set_smbus_config command.
DEFINE_int32(smbus_speed, -1,
             "The SMBus speed for the set_smbus_config command");
DEFINE_int32(smbus_address, -1,
             "The SMBus address for the set_smbus_config command");
DEFINE_int32(smbus_write_timeout_ms, -1,
             "The SMBus write timeout for the set_smbus_config command");
DEFINE_int32(smbus_read_timeout_ms, -1,
             "The SMBus read timeout for the set_smbus_config command");
DEFINE_int32(scl_low_timeout, -1,
             "Whether to enable the SCL low timeout for the "
             "set_smbus_config command");
DEFINE_int32(smbus_retry_limit, -1,
             "The SMBus retry limit for the set_smbus_config command");

class ArgError : public std::exception {
 public:
  template<typename... Args>
  explicit ArgError(const Args&... args)
    : what_(folly::to<std::string>(args...)) {}

  const char* what() const noexcept override {
    return what_.c_str();
  }

 private:
  std::string what_;
};

void hexdump(const uint8_t* buf, size_t length) {
  size_t idx = 0;
  while (idx < length) {
    printf("%04zx:", idx);
    for (unsigned int n = 0; n < 16 && idx < length; ++n, ++idx) {
      printf("%s%02x", n == 8 ? "  " : " ", buf[idx]);
    }
    printf("\n");
  }
}

long parseInt(StringPiece str, StringPiece name, long min, long max) {
  // We use strtol() here instead of folly::to since we want to
  // allow hex strings.
  const char* begin = str.begin();
  char* end = const_cast<char*>(str.end());
  auto value = strtol(begin, &end, 0);
  if (end == begin || end != str.end()) {
    throw ArgError("bad ", name, " ", str, ": must be an integer");
  }
  if (value < min) {
    throw ArgError("bad ", name, " ", str, ": cannot be smaller than ", min);
  }
  if (value > max) {
    throw ArgError("bad ", name, " ", str, ": cannot be larger than ", max);
  }

  return value;
}

typedef int (*CommandFn)(CP2112*, const std::vector<StringPiece>& args);

int cmdVersion(CP2112* dev, const vector<StringPiece>& args) {
  if (!args.empty()) {
    fprintf(stderr, "error: unexpected trailing arguments\n");
    return 1;
  }

  auto info = dev->getVersion();
  printf("Part Number:    %#04x\n", info.partNumber);
  printf("Device Version: %#04x\n", info.version);
  return 0;
}

void printGpios(const char* label, int labelWidth, uint8_t value) {
  char buf[17];
  for (int n = 0; n < 8; ++n) {
    bool set = ((value >> n) & 0x1);
    buf[2 * n] = ' ';
    buf[(2 * n) + 1] = set ? '1' : '0';
  }
  buf[16] = '\0';
  printf("%*s %s\n", labelWidth, label, buf);
}

int cmdGetGpioConfig(CP2112* dev, const vector<StringPiece>& args) {
  if (!args.empty()) {
    fprintf(stderr, "error: unexpected trailing arguments\n");
    return 1;
  }

  auto config = dev->getGpioConfig();
  printf("%14s  0 1 2 3 4 5 6 7\n", "GPIO:");
  printGpios("Direction:", 14, config.direction);
  printGpios("Push-pull:", 14, config.pushPull);

  std::string specialInfo;
  if (config.special) {
    bool first = true;
    specialInfo = " (";
    if (config.special & 0x1) {
      specialInfo += "GPIO7_CLK";
      first = false;
    }
    if (config.special & 0x2) {
      if (!first) {
        specialInfo += ", ";
      }
      specialInfo += "GPIO0_TXT";
      first = false;
    }
    if (config.special & 0x3) {
      if (!first) {
        specialInfo += ", ";
      }
      specialInfo += "GPIO1_RXT";
      first = false;
    }
    specialInfo += ")";
  }

  printf("      Special:  0x%02x%s\n", config.special, specialInfo.c_str());
  printf("Clock Divider:  0x%02x\n", config.clockDivider);
  return 0;
}

template<typename INTTYPE, typename INTTYPE2>
void updateConfig(INTTYPE* value,
                  StringPiece flagName,
                  int32_t flagValue,
                  INTTYPE2 max) {
  if (flagValue < 0) {
    return;
  }

  if (static_cast<uint32_t>(flagValue) > max) {
    throw ArgError("--", flagName, " cannot be larger than ", max);
  }
  *value = flagValue;
}

template<typename INTTYPE>
void updateConfig(INTTYPE* value,
                  StringPiece flagName,
                  int32_t flagValue) {
  updateConfig(value, flagName, flagValue,
               std::numeric_limits<INTTYPE>::max());
}

#define UPDATE_CONFIG(value, flag, ...) \
  updateConfig(&(value), #flag, FLAGS_ ## flag, ##__VA_ARGS__);

int cmdSetGpioConfig(CP2112* dev, const vector<StringPiece>& args) {
  if (!args.empty()) {
    fprintf(stderr, "error: unexpected trailing arguments\n");
    return 1;
  }

  // Start with the current config settings
  auto config = dev->getGpioConfig();
  UPDATE_CONFIG(config.direction, gpio_direction);
  UPDATE_CONFIG(config.pushPull, gpio_push_pull);
  UPDATE_CONFIG(config.special, gpio_special);
  UPDATE_CONFIG(config.clockDivider, gpio_clock_divider);

  dev->setGpioConfig(config);
  return 0;
}

int cmdEnableRxTxLeds(CP2112* dev, const vector<StringPiece>& args) {
  if (!args.empty()) {
    fprintf(stderr, "error: unexpected trailing arguments\n");
    return 1;
  }

  auto config = dev->getGpioConfig();
  config.direction |= 0x03; // GPIO 0 and 1 set to output
  config.pushPull |= 0x03;  // Push-pull for GPIO 0 and 1
  config.special |= 0x06;   // Enable TX/RX toggle for GPIOs 0 and 1
  dev->setGpioConfig(config);
  return 0;
}

int cmdGetGpio(CP2112* dev, const vector<StringPiece>& args) {
  if (!args.empty()) {
    fprintf(stderr, "error: unexpected trailing arguments\n");
    return 1;
  }

  uint8_t values = dev->getGpio();
  printf("%6s  0 1 2 3 4 5 6 7\n", "GPIO:");
  printGpios("Value:", 6, values);
  return 0;
}

int cmdSetGpio(CP2112* dev, const vector<StringPiece>& args) {
  if (args.size() != 2) {
    fprintf(stderr, "error: incorrect number of arguments\n");
    fprintf(stderr, "usage: cp2112_util set_gpio VALUES MASK\n");
    return 1;
  }

  uint8_t value = parseInt(args[0], "value", 0, 0xff);
  uint8_t mask = parseInt(args[1], "mask", 0, 0xff);
  dev->setGpio(value, mask);
  return 0;
}

int cmdGetSMBusConfig(CP2112* dev, const vector<StringPiece>& args) {
  if (!args.empty()) {
    fprintf(stderr, "error: unexpected trailing arguments\n");
    return 1;
  }

  auto config = dev->getSMBusConfig();
  printf("Speed:           %d\n", config.speed);
  printf("Address:         %#04x\n", config.address);
  printf("Auto-send Read:  %d\n", config.autoSendRead);
  printf("Write Timeout:   %dms\n", config.writeTimeoutMS);
  printf("Read Timeout:    %dms\n", config.readTimeoutMS);
  printf("SCL Low Timeout: %d\n", config.sclLowTimeout);
  printf("Retry Limit:     %d\n", config.retryLimit);
  return 0;
}

int cmdSetSMBusConfig(CP2112* dev, const vector<StringPiece>& args) {
  if (!args.empty()) {
    fprintf(stderr, "error: unexpected trailing arguments\n");
    return 1;
  }

  // Start with the current config settings
  auto config = dev->getSMBusConfig();
  UPDATE_CONFIG(config.speed, smbus_speed);
  UPDATE_CONFIG(config.address, smbus_address);
  // The CP2112 code relies on autoSendRead always being 0
  // We don't allow this value to be changed for now.
  config.autoSendRead = 0;
  UPDATE_CONFIG(config.writeTimeoutMS, smbus_write_timeout_ms, 1000);
  UPDATE_CONFIG(config.readTimeoutMS, smbus_read_timeout_ms, 1000);
  UPDATE_CONFIG(config.sclLowTimeout, scl_low_timeout, 1);
  UPDATE_CONFIG(config.retryLimit, smbus_retry_limit, 1000);

  dev->setSMBusConfig(config);
  return 0;
}

int cmdRead(CP2112* dev, const vector<StringPiece>& args) {
  if (args.size() != 2) {
    fprintf(stderr, "error: incorrect number of arguments\n");
    fprintf(stderr, "usage: cp2112_util read ADDRESS LENGTH\n");
    return 1;
  }

  uint8_t address = parseInt(args[0], "address", 0, 0xff);
  if (address & 0x1) {
    fprintf(stderr, "error: bad address %s: least significant bit must be 0\n",
            args[0].str().c_str());
    return 1;
  }

  uint16_t length = parseInt(args[1], "length", 0, 0xffff);

  uint8_t buf[length];
  memset(buf, 0x55, length);
  try {
    dev->read(address, MutableByteRange(buf, length));
  } catch (const std::exception& ex) {
    fprintf(stderr, "error performing read: %s\n", ex.what());
    return 2;
  }

  printf("Read %d bytes at address %#04x:\n", length, address);
  hexdump(buf, length);
  return 0;
}

int addrReadImpl(CP2112* dev, const vector<StringPiece>& args,
                 bool unsafe) {
  if (args.size() != 3) {
    fprintf(stderr, "error: incorrect number of arguments\n");
    fprintf(stderr, "usage: cp2112_util read DEV_ADDRESS ADDRESS LENGTH\n");
    return 1;
  }

  uint8_t address = parseInt(args[0], "address", 0, 0xff);
  if (address & 0x1) {
    fprintf(stderr, "error: bad address %s: least significant bit must be 0\n",
            args[0].str().c_str());
    return 1;
  }

  // CP2112 supports target addresses of up to 16 bytes.
  // For now we only support using 1 byte.  (I've been testing with
  // a 24c08 EEPROM which uses a 1 byte target address.)
  uint8_t targetAddr = parseInt(args[1], "address", 0, 0xff);
  if (address & 0x1) {
    fprintf(stderr, "error: bad address %s: least significant bit must be 0\n",
            args[0].str().c_str());
    return 1;
  }

  uint16_t length = parseInt(args[2], "length", 0, 0xffff);

  ByteRange addrBytes(&targetAddr, 1);
  uint8_t buf[length];
  memset(buf, 0x55, length);
  try {
    // Only use CP2112::writeReadUnsafe() if the unsafe flag is set.
    // This performs a read-after-write using a repeated start to avoid
    // releasing the bus in between the operations.  Unfortunately, it seems
    // to cause the CP2112 device to lock up if the operation times out,
    // so we generally avoid using it.
    if (unsafe) {
      dev->writeReadUnsafe(address, addrBytes, MutableByteRange(buf, length));
    } else {
      dev->write(address, addrBytes);
      dev->read(address, MutableByteRange(buf, length));
    }
  } catch (const std::exception& ex) {
    fprintf(stderr, "error performing read: %s\n", ex.what());
    return 2;
  }

  printf("Read %d bytes at device 0x%02x address 0x%02x:\n",
         length, address, targetAddr);
  hexdump(buf, length);
  return 0;
}

int cmdAddrRead(CP2112* dev, const vector<StringPiece>& args) {
  return addrReadImpl(dev, args, false);
}

int cmdAddrReadUnsafe(CP2112* dev, const vector<StringPiece>& args) {
  return addrReadImpl(dev, args, true);
}

int cmdWrite(CP2112* dev, const vector<StringPiece>& args) {
  if (args.size() < 2) {
    fprintf(stderr, "error: incorrect number of arguments\n");
    fprintf(stderr, "usage: cp2112_util write ADDRESS BYTE1 [BYTE2 ...]\n");
    return 1;
  }

  uint8_t address = parseInt(args[0], "address", 0, 0xff);
  if (address & 0x1) {
    fprintf(stderr, "error: bad address %s: least significant bit must be 0\n",
            args[0].str().c_str());
    return 1;
  }

  auto length = args.size() - 1;
  uint8_t buf[length];
  for (size_t n = 1; n < args.size(); ++ n) {
    buf[n - 1] = parseInt(args[n], "byte", 0, 0xff);
  }

  printf("Writing %zu bytes to address %#04x:\n", length, address);
  hexdump(buf, length);

  dev->write(address, ByteRange(buf, length));
  return 0;
}

bool scanAddress(CP2112* dev, uint8_t address) {
  if (address == 0 || address == 2) {
    // Don't try scanning address 0 (general call address, which addresses all
    // devices), or address 2 (which is the slave address of the CP2112
    // itself).
    return false;
  }

  // We send a 1 byte read to detect if a device is present.
  //
  // Note: The i2cdetect program from i2c-tools uses a 1-byte read in most
  // cases, but a quick write (no write data, just the 1 R/W bit) for addresses
  // 0x30-0x37 and 0x50-0x5f.  Apparently there are some write-only devices
  // with addresses in this range that will lock up the bus if sent a read.
  uint16_t length = 1;
  uint8_t buf[length];
  try {
    dev->read(address, MutableByteRange(buf, length));
  } catch (const std::exception& ex) {
    return false;
  }

  return true;
}

int cmdDetect(CP2112* dev, const vector<StringPiece>& args) {
  if (!args.empty()) {
    fprintf(stderr, "error: unexpected trailing arguments\n");
    return 1;
  }

  if (FLAGS_timeout_ms <= 0) {
    std::chrono::milliseconds timeout{20};
    dev->setDefaultTimeout(timeout);
  }

  fprintf(stdout, "      00 02 04 06 08 0a 0c 0e\n");
  for (uint8_t i = 0; i < 128; i += 8) {
    fprintf(stdout, "0x%02x:", i << 1);
    for (uint8_t j = 0; j < 8; ++j) {
      uint8_t addr = (i + j) << 1;
      if (scanAddress(dev, addr)) {
        fprintf(stdout, " %02x", addr);
      } else {
        fprintf(stdout, " --");
      }
      fflush(stdout);
    }
    fprintf(stdout, "\n");
  }
  return 0;
}

int cmdReset(CP2112* dev, const vector<StringPiece>& args) {
  if (!args.empty()) {
    fprintf(stderr, "error: unexpected trailing arguments\n");
    return 1;
  }

  dev->resetDevice();
  return 0;
}

void printXferStatus(const CP2112::TransferStatus& status) {
  printf("Status 0:        %d (%s)\n",
         status.status0, CP2112::getStatus0Msg(status.status0).c_str());
  if (status.status0 == 0) {
    return;
  }
  printf("Status 1:        %d (%s)\n",
         status.status1,
         CP2112::getStatus1Msg(status.status0, status.status1).c_str());
  printf("Num Retries:     %d\n", status.numRetries);
  printf("Bytes Read:      %d\n", status.bytesRead);
}

int cmdCancelXfer(CP2112* dev, const vector<StringPiece>& args) {
  if (!args.empty()) {
    fprintf(stderr, "error: unexpected trailing arguments\n");
    return 1;
  }

  auto status = dev->cancelTransfer();
  printf("Cancelled transfer; current status:\n");
  printXferStatus(status);
  return 0;
}

int cmdXferStatus(CP2112* dev, const vector<StringPiece>& args) {
  if (!args.empty()) {
    fprintf(stderr, "error: unexpected trailing arguments\n");
    return 1;
  }

  auto status = dev->getTransferStatus();
  printXferStatus(status);
  return 0;
}

struct CommandInfo {
  CommandFn fn;
  const char* usage;
};

std::map<std::string, CommandInfo> kCommands = {
  {"addr_read", {cmdAddrRead, " DEV_ADDRESS ADDRESS LENGTH"}},
  {"addr_read_unsafe", {cmdAddrReadUnsafe, " DEV_ADDRESS ADDRESS LENGTH"}},
  {"cancel_xfer", {cmdCancelXfer, ""}},
  {"detect", {cmdDetect, ""}},
  {"get_gpio", {cmdGetGpio, ""}},
  {"set_gpio", {cmdSetGpio, "VALUES MASK"}},
  {"get_gpio_config", {cmdGetGpioConfig, ""}},
  {"set_gpio_config", {cmdSetGpioConfig, ""}},
  {"get_smbus_config", {cmdGetSMBusConfig, ""}},
  {"set_smbus_config", {cmdSetSMBusConfig, ""}},
  {"read", {cmdRead, " ADDRESS LENGTH"}},
  {"reset", {cmdReset, ""}},
  {"tx_rx_leds", {cmdEnableRxTxLeds, ""}},
  {"version", {cmdVersion, ""}},
  {"write", {cmdWrite, " ADDRESS BYTE1 [BYTE2 ...]"}},
  {"xfer_status", {cmdXferStatus, ""}},
};

void printCommandList(FILE* f) {
  fprintf(f, "\nValid commands:\n");
  for (const auto& entry : kCommands) {
    fprintf(f, "    %s%s\n", entry.first.c_str(), entry.second.usage);
  }
}

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  gflags::SetCommandLineOptionWithMode(
      "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);

  if (FLAGS_list_commands) {
    printCommandList(stdout);
    return 0;
  }

  if (argc < 2) {
    fprintf(stderr, "error: no command specified\n");
    fprintf(stderr, "usage: %s COMMAND\n", argv[0]);
    printCommandList(stderr);
    return 1;
  }

  auto it = kCommands.find(argv[1]);
  if (it == kCommands.end()) {
    fprintf(stderr, "error: unknown command \"%s\"\n", argv[1]);
    printCommandList(stderr);
    return 1;
  }
  CommandFn cmdFn = it->second.fn;
  vector<StringPiece> extraArgs;
  for (int n = 2; n < argc; ++n) {
    extraArgs.emplace_back(argv[n]);
  }

  CP2112 dev;
  try {
    dev.open(FLAGS_init_smbus_config);
  } catch (const std::exception& ex) {
    fprintf(stderr, "error: unable to open device: %s\n", ex.what());
    return 1;
  }

  if (FLAGS_reset) {
    printf("Resetting device...");
    fflush(stdout);
    dev.resetDevice();
    printf("done\n");
  }
  if (FLAGS_timeout_ms > 0) {
    std::chrono::milliseconds timeout(FLAGS_timeout_ms);
    dev.setDefaultTimeout(timeout);
  }

  int rc;
  try {
    rc = cmdFn(&dev, extraArgs);
  } catch (const ArgError& ex) {
    fprintf(stderr, "error: %s\n", ex.what());
    return 1;
  }

  return rc;
}
