// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/platform/helpers/Utils.h"
#include <exprtk.hpp>
#include <fcntl.h>
#include <folly/Subprocess.h>
#include <folly/system/Shell.h>
#include <re2/re2.h>
#include <sys/mman.h>
#include <unistd.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <unordered_set>

namespace {
constexpr uint32_t MAP_SIZE = 4096;
constexpr uint32_t MAP_MASK = MAP_SIZE - 1;

const std::unordered_set<std::string> kFlashType = {
    "MX25L12805D",
    "N25Q128..3E",
};

using namespace folly::literals::shell_literals;

std::string execCommandImpl(const std::string& cmd, int* exitStatus) {
  auto shellCmd = "/bin/sh -c {}"_shellify(cmd);
  folly::Subprocess p(shellCmd, folly::Subprocess::Options().pipeStdout());
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

uint64_t nowInSecs() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

float computeExpression(
    const std::string& equation,
    float input,
    const std::string& symbol) {
  std::string temp_equation = equation;

  // Replace "@" with a valid symbol
  static const re2::RE2 atRegex("@");

  re2::RE2::GlobalReplace(&temp_equation, atRegex, symbol);

  exprtk::symbol_table<float> symbolTable;

  symbolTable.add_variable(symbol, input);

  exprtk::expression<float> expr;
  expr.register_symbol_table(symbolTable);

  exprtk::parser<float> parser;
  parser.compile(temp_equation, expr);

  return expr.value();
}

} // namespace facebook::fboss::platform::helpers
