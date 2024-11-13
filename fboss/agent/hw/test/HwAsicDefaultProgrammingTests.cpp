// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <map>
#include <string>

#include <folly/String.h>
#include <gtest/gtest.h>
#include <re2/re2.h>

#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/golden_data.h" // provides golden_dataMap

using namespace ::testing;

DEFINE_string(dump_golden_data, "", "File to dump expected golden data to");

namespace facebook::fboss {

// This test aims to detect unintended ASIC programming changes. It dumps
// default ASIC programming and compares them against a golden data file. If any
// diff is detected, we should manually check and ensure the change is benign.
//
// To refresh the golden data:
// 1. ./sai_test-<sdk> --config=<config.json> \
//     --gtest_filter=*verifyDefaultProgramming \
//     --dump_golden_data=<platform.csv>
// 2. scp <platform.csv> fboss/agent/hw/test/golden/asic
//
class HwAsicDefaultProgrammingTest : public HwTest {
 protected:
  std::map<std::string, std::string> loadGoldenData(cfg::AsicType asic) {
    std::string asicType = apache::thrift::util::enumNameSafe(asic);
    asicType = asicType.substr(std::string("ASIC_TYPE_").size());
    folly::toLowerAscii(asicType);
    std::string filename = "golden/asic/" + asicType + ".csv";

    XLOG(INFO) << "Loading golden data file: " << filename;
    auto data = golden_dataMap.find(filename);
    if (data == golden_dataMap.end()) {
      XLOG(WARN) << "Golden data not found. Embedded data:";
      for (const auto& [key, value] : golden_dataMap) {
        XLOG(INFO) << "data[" << key << "]: " << value.size() << " bytes";
      }
      throw std::invalid_argument("Golden data not found: '" + asicType + "'");
    }

    // Use an ordered map so that errors are sorted and are more readable.
    std::map<std::string, std::string> ret;
    std::istringstream iss(std::string(data->second));
    std::string line;
    while (std::getline(iss, line)) {
      std::string type, key, value;
      folly::split(',', line, type, key, value);
      ret[type + ":" + key] = folly::trimWhitespace(value);
    }
    return ret;
  }

  std::map<std::string, std::string> fetchData(cfg::AsicType asic) {
    static const std::
        map<cfg::AsicType, std::set<std::pair<std::string, std::string>>>
            kQueries = {
                {cfg::AsicType::ASIC_TYPE_JERICHO3,
                 {
                     {"mem", "CGM_VOQ_DRAM_BOUND_PRMS"},
                     {"mem", "CGM_VOQ_DRAM_RECOVERY_PRMS"},
                     {"mem", "CGM_VSQF_FC_PRMS"},
                     {"mem", "CGM_VSQE_RJCT_PRMS"},
                     {"mem", "CGM_PB_VSQ_RJCT_MASK"},
                     {"mem", "IPS_CRBAL_TH"},
                     {"mem", "IPS_EMPTY_Q_CRBAL_TH"},
                     {"mem", "IPS_QSIZE_TH"},
                     {"mem", "SCH_DEVICE_RATE_MEMORY_DRM"},
                     {"mem", "SCH_SHARED_DEVICE_RATE_SHARED_DRM"},

                     {"reg", "CIG_RCI_CONFIGS"},
                     {"reg", "CIG_RCI_CORE_MAPPING"},
                     {"reg", "CIG_RCI_DEVICE_MAPPING"},
                     {"reg", "CIG_RCI_FDA_OUT_TH"},
                     {"reg", "CIG_RCI_FDA_OUT_TOTAL_TH"},
                     {"reg", "FDA_OFM_CORE_FIFO_CONFIG_CORE"},
                     {"reg", "FDTS_FDT_SHAPER_CONFIGURATIONS"},

                     // This is not stable, it differs from run to run:
                     // - FDT_LOAD_BALANCING_SWITCH_CONFIGURATION
                 }},
            };
    auto queries = kQueries.find(asic);

    // "Prime" the diag shell because sometimes the first command fails.
    std::string diagOut;
    getHwSwitchEnsemble()->runDiagCommand("h", diagOut);

    std::map<std::string, std::string> ret;
    for (const auto& [type, var] : queries->second) {
      std::string cmd;
      if (type == "mem") {
        cmd = "dump raw " + var + "\n";
      } else if (type == "reg") {
        cmd = "getreg raw " + var + "\n";
      }

      getHwSwitchEnsemble()->runDiagCommand(cmd, diagOut);

      std::istringstream iss(diagOut);
      std::string line;
      while (std::getline(iss, line)) {
        // Example: CIG_RCI_CONFIGS.CIG0[0x103]=0x5140e101f407d081ac
        static const re2::RE2 pattern("([a-zA-Z0-9_\\.\\[\\]]+)\\s*[:=](.*)");
        std::string key, value;
        if (re2::RE2::FullMatch(line, pattern, &key, &value)) {
          ret[type + ":" + key] = folly::trimWhitespace(value);
        } else {
          // Ignored any junk echoed by the diag shell.
          XLOG(DBG3) << "Ignored unparsable output '" << line << "'";
        }
      }
    }
    getHwSwitchEnsemble()->runDiagCommand("quit\n", diagOut);
    return ret;
  }
};

TEST_F(HwAsicDefaultProgrammingTest, verifyDefaultProgramming) {
  auto golden = loadGoldenData(getAsicType());
  auto fetched = fetchData(getAsicType());

  // Do a manual comparison in order to emit better error messages.
  for (const auto& [key, goldenValue] : golden) {
    auto fetchedValue = fetched.find(key);
    if (fetchedValue != fetched.end()) {
      EXPECT_EQ(fetchedValue->second, goldenValue) << "Diff in key: " << key;
    } else {
      ADD_FAILURE() << "Missing key in fetched data: " << key << "="
                    << goldenValue;
    }
  }
  for (const auto& [key, fetchedValue] : fetched) {
    EXPECT_TRUE(golden.contains(key))
        << "Extra key in fetched data: " << key << "=" << fetchedValue;
  }

  // Dump golden data if requested.
  if (!FLAGS_dump_golden_data.empty()) {
    std::ofstream ofs(FLAGS_dump_golden_data);
    for (const auto& [key, value] : fetched) {
      std::string type, var;
      folly::split(':', key, type, var);
      ofs << type << "," << var << "," << value << "\n";
    }
  }
}

} // namespace facebook::fboss
