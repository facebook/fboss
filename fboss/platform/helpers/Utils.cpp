// Copyright 2004-present Facebook. All Rights Reserved.

#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>
#include <unordered_set>

#include <folly/Subprocess.h>

namespace {
constexpr uint32_t MAP_SIZE = 4096;
constexpr uint32_t MAP_MASK = MAP_SIZE - 1;

const std::unordered_set<std::string> kFlashType = {
    "MX25L12805D",
    "N25Q128..3E",
};

std::string execCommandImpl(const std::string& cmd, int* exitStatus) {
  folly::Subprocess p({cmd}, folly::Subprocess::Options().pipeStdout());
  auto result = p.communicate();
  if (exitStatus) {
    *exitStatus = p.wait().exitStatus();
  } else {
    p.waitChecked();
  }
  return result.first;
}
} // namespace
namespace facebook::fboss::platform::helpers {

std::string execCommand(const std::string& cmd) {
  return execCommandImpl(cmd, nullptr);
}

std::string execCommandUnchecked(const std::string& cmd, int& exitStatus) {
  return execCommandImpl(cmd, &exitStatus);
}

/*
 * mmap function to read memory mapped address
 */

uint32_t mmap_read(uint32_t address, char acc_type) {
  int fd;
  void *map_base, *virt_addr;
  uint32_t read_result;

  if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
    throw std::runtime_error("Cannot open /dev/mem");
  }

  map_base = mmap(
      nullptr,
      MAP_SIZE,
      PROT_READ | PROT_WRITE,
      MAP_SHARED,
      fd,
      address & ~MAP_MASK);

  if (map_base == (void*)-1) {
    close(fd);
    throw std::runtime_error("mmap failed");
  }

  // To avoid "error: arithmetic on a pointer to void"
  virt_addr = (void*)((char*)map_base + (address & MAP_MASK));

  switch (acc_type) {
    case 'b':
      read_result = *((unsigned char*)virt_addr);
      break;
    case 'h':
      read_result = *((uint16_t*)virt_addr);
      break;
    case 'w':
      read_result = *((uint32_t*)virt_addr);
      break;
  }

  std::cout << "Value at address 0x" << std::hex << address << " (0x"
            << virt_addr << "): 0x" << read_result << std::endl;

  if (munmap(map_base, MAP_SIZE) == -1) {
    close(fd);
    throw std::runtime_error("mmap failed");
  }

  close(fd);

  return read_result;
}

/*
 * mmap function to read memory mapped address
 */
int mmap_write(uint32_t address, char acc_type, uint32_t val) {
  int fd;
  void *map_base, *virt_addr;
  uint32_t read_result;

  if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
    throw std::runtime_error("Cannot open /dev/mem");
  }

  map_base = mmap(
      nullptr,
      MAP_SIZE,
      PROT_READ | PROT_WRITE,
      MAP_SHARED,
      fd,
      address & ~MAP_MASK);

  if (map_base == (void*)-1) {
    close(fd);
    throw std::runtime_error("mmap failed");
  }

  // To avoid "error: arithmetic on a pointer to void"
  virt_addr = (void*)((char*)map_base + (address & MAP_MASK));

  read_result = *((uint32_t*)virt_addr);

  std::cout << "Value at address 0x" << std::hex << address << " (0x"
            << virt_addr << "): 0x" << read_result << std::endl;

  switch (acc_type) {
    case 'b':
      *((unsigned char*)virt_addr) = val;
      read_result = *((unsigned char*)virt_addr);
      break;
    case 'h':
      *((uint16_t*)virt_addr) = val;
      read_result = *((uint16_t*)virt_addr);
      break;
    case 'w':
      *((uint32_t*)virt_addr) = val;
      read_result = *((uint32_t*)virt_addr);
      break;
  }

  std::cout << "Written 0x" << std::hex << val << "; readback 0x" << read_result
            << std::endl;

  if (munmap(map_base, MAP_SIZE) == -1) {
    close(fd);
    throw std::runtime_error("mmap failed");
  }

  close(fd);

  return 0;
}

std::string getFlashType(const std::string& str) {
  for (auto& it : kFlashType) {
    if (str.find(it) != std::string::npos) {
      return it;
    }
  }
  return "";
}

} // namespace facebook::fboss::platform::helpers
