// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <fcntl.h>
#include <folly/Conv.h>
#include <folly/String.h>
#include <folly/init/Init.h>
#include <gflags/gflags.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sysexits.h>
#include <unistd.h>
#include <sstream>
#include <string>

DEFINE_bool(
    read_reg,
    false,
    "Read register from device, use with --offset [--length]");
DEFINE_int32(offset, 0, "Device register address");
DEFINE_int32(length, 1, "Number of registers to read");
DEFINE_bool(
    write_reg,
    false,
    "Write register from device, use with --offset --data");
DEFINE_uint32(data, -1, "Register data for write");
DEFINE_int32(pim, 0, "PIM Id 2..9 (PIM index 0..7 maps to  PIM Id 2..9)");

struct csc_info_s {
  unsigned int mapSize;
  unsigned char* mapAddr;
};

uint32_t pimBaseAddress[8] = {
    0x0040100, // PIM 2
    0x0441100, // PIM 3
    0x0842100, // PIM 4
    0x0C43100, // PIM 5
    0x1044100, // PIM 6
    0x1445100, // PIM 7
    0x1846100, // PIM 8
    0x1C47100, // PIM 9
};

/*
 * cscReadRegInternal
 *
 * Read the SMB/PIM FPGA register. The register address is 32 bit value, the
 * data read is 32 bit value. For PUM the register address includes PIM base
 * offset
 */
unsigned int cscReadRegInternal(struct csc_info_s* pMem, uint32_t regAddr) {
  unsigned char* ptr = pMem->mapAddr;
  if (regAddr >= pMem->mapSize) {
    // Reg offset should not be more than map size
    throw std::runtime_error("Register offset exceeds memory map size");
  }
  ptr += regAddr;
  return *(unsigned int*)ptr;
}

/*
 * cscReadReg
 *
 * Reads the SMB/PIM FPGA register.
 */
bool cscReadReg(struct csc_info_s* pMem, uint32_t regAddr, uint32_t length) {
  unsigned int regVal;

  for (int i = 0; i < length; i++) {
    regVal = cscReadRegInternal(pMem, regAddr + i * 4);
    if (!(i % 4)) {
      if (i) {
        printf("\n");
      }
      printf("0x%x: ", regAddr + i * 4);
    }
    printf("%.8x ", regVal);
  }
  printf("\n");
  return true;
}

/*
 * cscWriteRegInternal
 *
 * Write the SMB/PIM FPGA register. The register address is 32 bit value, the
 * data is 32 bit value. For PUM the register address includes PIM base
 * offset
 */
void cscWriteRegInternal(
    struct csc_info_s* pMem,
    uint32_t regAddr,
    uint32_t regVal) {
  unsigned char* ptr = pMem->mapAddr;
  if (regAddr >= pMem->mapSize) {
    return;
  }
  ptr += regAddr;
  *(unsigned int*)ptr = regVal;
}

/*
 * cscWriteReg
 *
 * Writes to the SMB/PIM FPGA register.
 */
bool cscWriteReg(struct csc_info_s* pMem, uint32_t regAddr, uint32_t regVal) {
  cscWriteRegInternal(pMem, regAddr, regVal);
  regVal = cscReadRegInternal(pMem, regAddr);
  printf("Reg 0x%x = 0x%.8x\n", regAddr, regVal);
  return true;
}

/*
 * csc_fpga_util
 *
 * This utility finds the SMB FPGA on PCI link in Sandia platform. It then
 * allows user to read/write to the SMB/PIM FPGA registers
 */
int main(int argc, char* argv[]) {
  folly::init(&argc, &argv, true);
  gflags::SetCommandLineOptionWithMode(
      "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);

  std::string sysfsPath;
  std::array<char, 64> busId;

  // Sandia SMB FPGA device id is 0x017a
  FILE* fp = popen("lspci -d  1137:017a | sed -e \'s/ .*//\'", "r");
  if (!fp) {
    printf("csc FPGA not found on PCI bus\n");
    return EX_SOFTWARE;
  }
  fgets(busId.data(), 64, fp);
  std::istringstream iStream(folly::to<std::string>(busId));
  std::string busIdStr;
  iStream >> busIdStr;

  // Build the sysfs file path for resource0 where all registers are mapped
  sysfsPath = folly::to<std::string>(
      "/sys/bus/pci/devices/0000:", busIdStr, "/resource0");

  pclose(fp);

  // Open the sysfs file to access the device
  int fd = open(sysfsPath.c_str(), O_RDWR | O_SYNC);
  if (fd == -1) {
    printf("Unable to open up the sysfs file %s\n", sysfsPath.c_str());
    return EX_SOFTWARE;
  }

  // Mmeory map the device's 64MB memory register space to our
  // user space
  struct csc_info_s cscInfo;
  cscInfo.mapSize = 0x4000000;
  cscInfo.mapAddr = (unsigned char*)mmap(
      nullptr, 0x4000000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (cscInfo.mapAddr == MAP_FAILED) {
    printf("mmap failed for resource\n");
    close(fd);
    return EX_SOFTWARE;
  }

  uint32_t pimBase = 0;
  if (FLAGS_pim >= 2 && FLAGS_pim <= 9) {
    pimBase = pimBaseAddress[FLAGS_pim - 2];
  }

  if (FLAGS_read_reg) {
    printf(
        "Calling cscReadReg with address 0x%x length %d\n",
        pimBase + FLAGS_offset,
        FLAGS_length);
    cscReadReg(&cscInfo, pimBase + FLAGS_offset, FLAGS_length);
  }
  if (FLAGS_write_reg) {
    if (FLAGS_data == static_cast<unsigned int>(-1)) {
      printf("Valid data need to be specified for write operation\n");
      munmap(cscInfo.mapAddr, 0x4000000);
      close(fd);
      return EX_SOFTWARE;
    }
    printf(
        "Calling cscWriteReg with address 0x%x value 0x%x\n",
        pimBase + FLAGS_offset,
        FLAGS_data);
    cscWriteReg(&cscInfo, pimBase + FLAGS_offset, FLAGS_data);
  }

  munmap(cscInfo.mapAddr, 0x4000000);
  close(fd);
  return EX_OK;
}
