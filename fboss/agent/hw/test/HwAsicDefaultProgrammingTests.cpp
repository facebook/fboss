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
//     --dump_golden_data=<platform>-<version>.csv
// 2. scp <csv> fboss/agent/hw/test/golden/asic
//
class HwAsicDefaultProgrammingTest : public HwTest {
 protected:
  std::string querySdkMajorVersion() {
    std::string version;

    // Extend to other SDKs if needed.
    static const re2::RE2 bcmSaiPattern("BRCM SAI ver: \\[(\\d+)\\.");
    std::string out;
    getHwSwitchEnsemble()->runDiagCommand("bcmsai ver\n", out);
    if (RE2::PartialMatch(out, bcmSaiPattern, &version)) {
      return version;
    }

    return "default";
  }

  std::optional<std::map<std::string, std::string>> loadGoldenData(
      cfg::AsicType asic,
      const std::string& version) {
    std::string asicType = apache::thrift::util::enumNameSafe(asic);
    asicType = asicType.substr(std::string("ASIC_TYPE_").size());
    folly::toLowerAscii(asicType);
    std::string filename =
        fmt::format("golden/asic/{}-{}.csv", asicType, version);

    XLOG(INFO) << "Loading golden data file: " << filename;
    auto data = golden_dataMap.find(filename);
    if (data == golden_dataMap.end()) {
      XLOG(WARN) << "Golden data not found. Embedded data:";
      for (const auto& [key, value] : golden_dataMap) {
        XLOG(INFO) << key << ": " << value.size() << " bytes";
      }
      return std::nullopt;
    }

    // Use an ordered map so that errors are sorted and are more readable.
    std::map<std::string, std::string> ret;
    std::istringstream iss(std::string(data->second));
    std::string line;
    while (std::getline(iss, line)) {
      std::string type, key, value;
      // <false> means putting all remaining parts into the last field.
      if (!folly::split<false>(',', line, type, key, value)) {
        throw std::invalid_argument("Failed to parse: " + line);
      }
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
                     {"mem", "CGM_PB_VSQ_RJCT_MASK"},
                     {"mem", "CGM_VOQ_DRAM_BOUND_PRMS"},
                     {"mem", "CGM_VOQ_DRAM_RECOVERY_PRMS"},
                     {"mem", "CGM_VSQE_RJCT_PRMS"},
                     {"mem", "CGM_VSQF_FC_PRMS"},
                     {"mem", "CGM_VSQF_RJCT_PRMS"},
                     {"mem", "IPS_CRBAL_TH"},
                     {"mem", "IPS_EMPTY_Q_CRBAL_TH"},
                     {"mem", "IPS_QSIZE_TH"},
                     {"mem", "SCH_DEVICE_RATE_MEMORY_DRM"},
                     {"mem", "SCH_SHARED_DEVICE_RATE_SHARED_DRM"},

                     {"reg", "CGM_TOTAL_SRAM_RSRC_FLOW_CONTROL_THS"},
                     {"reg", "CIG_RCI_CONFIGS"},
                     {"reg", "CIG_RCI_CORE_MAPPING"},
                     {"reg", "CIG_RCI_DEVICE_MAPPING"},
                     {"reg", "CIG_RCI_FDA_OUT_TH"},
                     {"reg", "CIG_RCI_FDA_OUT_TOTAL_TH"},
                     {"reg", "FDA_OFM_CORE_FIFO_CONFIG_CORE"},
                     {"reg", "FDTS_FDT_SHAPER_CONFIGURATIONS"},
                     {"reg", "FMAC_LINK_LEVEL_FLOW_CONTROL_CONFIGURATIONS"},
                     {"reg", "RQP_RQP_AGING_CONFIG"},

                     // This is not stable, it differs from run to run:
                     // - FDT_LOAD_BALANCING_SWITCH_CONFIGURATION
                 }},
            };
    auto queries = kQueries.find(asic);

    std::string diagOut;
    std::map<std::string, std::string> ret;
    for (const auto& [type, var] : queries->second) {
      std::string cmd;
      if (type == "mem") {
        // Dump in the standard format instead of the raw format so that we can
        // filter out the ECC bits, also it's easier to inspect for diffs.
        cmd = "dump " + var + "\n";
      } else if (type == "reg") {
        cmd = "getreg " + var + "\n";
      }

      getHwSwitchEnsemble()->runDiagCommand(cmd, diagOut);

      std::istringstream iss(diagOut);
      std::string line;
      while (std::getline(iss, line)) {
        // mem example:
        // CGM_PB_VSQ_RJCT_MASK.CGM0[0]: <VOQ_RJCT_MASK=0x3ff,
        //   VSQ_RJCT_MASK=0xffffff, GLBL_RJCT_MASK=0xf>

        // reg example:
        // FDTS_FDT_SHAPER_CONFIGURATIONS.FDTS0[0x104]=0x500a0006428080a4176004e601:
        //   <FABRIC_SHAPER_EN=1, AUTO_DOC_NAME_23=0x27300,
        //   ...,
        //   FDT_SHAPER_CONFIGURATIONS_FIELD_6=0x14>
        static const re2::RE2 pattern(
            "([a-zA-Z0-9_()\\.\\[\\]]+)\\s*[:=]\\s*(.*)");
        std::string key, value;
        if (re2::RE2::FullMatch(line, pattern, &key, &value)) {
          static const re2::RE2 eccPattern(", ECC=[x0-9a-fA-F]+");
          re2::RE2::Replace(&value, eccPattern, "");
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
  auto setup = [&]() {};

  auto verify = [&]() {
    // Run a blank command since sometimes the first diag command gets ignored.
    std::string out;
    getHwSwitchEnsemble()->runDiagCommand("\n", out);

    auto golden = loadGoldenData(getAsicType(), querySdkMajorVersion());
    if (!golden.has_value()) {
      golden = loadGoldenData(getAsicType(), "default");
    }
    ASSERT_TRUE(golden.has_value()) << "No golden data found";

    auto fetched = fetchData(getAsicType());

    // Do a manual comparison in order to emit better error messages.
    for (const auto& [key, goldenValue] : *golden) {
      auto fetchedValue = fetched.find(key);
      if (fetchedValue != fetched.end()) {
        EXPECT_EQ(fetchedValue->second, goldenValue) << "Diff in key: " << key;
      } else {
        ADD_FAILURE() << "Missing key in fetched data: " << key << "="
                      << goldenValue;
      }
    }
    for (const auto& [key, fetchedValue] : fetched) {
      EXPECT_TRUE(golden->contains(key))
          << "Extra key in fetched data: " << key << " = " << fetchedValue;
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
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
