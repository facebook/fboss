// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/platformshowtech/utils.h"

namespace facebook::fboss::show::platformshowtech::utils {

int run_cmd(std::string cmd, std::string &output) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"),
                                                pclose);

  if (!pipe) {
    return -1;
  }

  while (std::fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  output = result;

  return 0;
}

std::string run_cmd_no_check(std::string cmd) {
  std::string output;

  run_cmd(cmd, output);
  return output;
}

std::string run_cmd_with_limit(std::string cmd, int max_lines) {
  std::string count_cmd = cmd + " | wc -l";
  int total_lines = std::stoi(run_cmd_no_check(count_cmd));

  if (total_lines <= max_lines) {
    return run_cmd_no_check(cmd);
  }

  int half_max = max_lines / 2;
  std::string first_part =
      run_cmd_no_check(cmd + " | head -n " + std::to_string(half_max));
  std::string last_part =
      run_cmd_no_check(cmd + " | tail -n " + std::to_string(half_max));

  std::string truncation_message =
      "=== File exceeds " + std::to_string(max_lines) +
      " lines (total: " + std::to_string(total_lines) +
      "). Showing first and last " + std::to_string(half_max) +
      " lines only ===\n\n";

  return truncation_message + first_part +
         "\n\n=== " + std::to_string(total_lines - max_lines) +
         " lines truncated ===\n\n" + last_part;
}

void print_fboss2_show_cmd(std::string cmd) {
  if (!std::filesystem::exists("/etc/ramdisk")) {
    std::cout << "#### fboss2 show " << cmd << " ####\n";
    std::cout << run_cmd_no_check("fboss2 show " + cmd) << std::endl;
  }
}

void strip(std::string &str) {
  str.erase(remove_if(str.begin(), str.end(), ::isspace), str.end());
}

int get_max_i2c_bus() {
  std::string output;

  if (run_cmd("/usr/sbin/i2cdetect -l | awk '{print $1}' | "
              "sed -e 's/i2c-//' | sort -n | tail -n 1",
              output)) {
    return 0;
  }

  strip(output);
  return std::stoi(output);
}

} // namespace facebook::fboss::show::platformshowtech::utils