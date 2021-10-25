// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/platform/helpers/utils.h"
#include <fcntl.h>
#include <glog/logging.h>
#include <sys/mman.h>
#include <iostream>

namespace facebook::fboss::platform::helpers {

/*
 * execCommand
 * Execute shell command and return output and exit status
 */
std::string execCommand(const std::string& cmd, int& out_exitStatus) {
  out_exitStatus = 0;
  auto pPipe = ::popen(cmd.c_str(), "r");
  if (pPipe == nullptr) {
    throw std::runtime_error("Cannot open pipe");
  }

  std::array<char, 256> buffer;

  std::string result;

  while (not std::feof(pPipe)) {
    auto bytes = std::fread(buffer.data(), 1, buffer.size(), pPipe);
    result.append(buffer.data(), bytes);
  }

  auto rc = ::pclose(pPipe);

  if (WIFEXITED(rc)) {
    out_exitStatus = WEXITSTATUS(rc);
  }

  return result;
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

} // namespace facebook::fboss::platform::helpers
