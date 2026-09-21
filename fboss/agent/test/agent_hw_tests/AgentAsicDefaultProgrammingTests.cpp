// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <map>
#include <string>

#include <folly/String.h>
#include <gtest/gtest.h>
#include <re2/re2.h>

#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/agent_hw_tests/golden_data.h"

using namespace ::testing;

DEFINE_string(dump_golden_data, "", "File to dump expected golden data to");

namespace facebook::fboss {

class AgentAsicDefaultProgrammingTest : public AgentHwTest {
 protected:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::HW_SWITCH};
  }

  std::string querySdkMajorVersion(SwitchID switchId) {
    std::string version;
    static const re2::RE2 bcmSaiPattern("BRCM SAI ver: \\[(\\d+)\\.");
    std::string out;
    getAgentEnsemble()->runDiagCommand("bcmsai ver\n", out, switchId);
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

    std::map<std::string, std::string> ret;
    std::istringstream iss(std::string(data->second));
    std::string line;
    while (std::getline(iss, line)) {
      std::string type, key, value;
      if (!folly::split<false>(',', line, type, key, value)) {
        throw std::invalid_argument("Failed to parse: " + line);
      }
      ret[fmt::format("{}:{}", type, key)] = folly::trimWhitespace(value);
    }
    return ret;
  }

  std::map<std::string, std::string> fetchData(
      cfg::AsicType asic,
      SwitchID switchId) {
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
                 }},
            };
    auto queries = kQueries.find(asic);
    if (queries == kQueries.end()) {
      return {};
    }

    std::string diagOut;
    std::map<std::string, std::string> ret;
    auto ensemble = getAgentEnsemble();
    for (const auto& [type, var] : queries->second) {
      std::string cmd;
      if (type == "mem") {
        cmd = "dump " + var + "\n";
      } else if (type == "reg") {
        cmd = "getreg " + var + "\n";
      }

      ensemble->runDiagCommand(cmd, diagOut, switchId);

      std::istringstream iss(diagOut);
      std::string line;
      while (std::getline(iss, line)) {
        static const re2::RE2 pattern(
            "([a-zA-Z0-9_()\\.\\[\\]]+)\\s*[:=]\\s*(.*)");
        std::string key, value;
        if (re2::RE2::FullMatch(line, pattern, &key, &value)) {
          static const re2::RE2 eccPattern(", ECC=[x0-9a-fA-F]+");
          re2::RE2::Replace(&value, eccPattern, "");
          ret[fmt::format("{}:{}", type, key)] = folly::trimWhitespace(value);
        } else {
          XLOG(DBG3) << "Ignored unparsable output '" << line << "'";
        }
      }
    }
    ensemble->runDiagCommand("quit\n", diagOut, switchId);
    return ret;
  }
};

TEST_F(AgentAsicDefaultProgrammingTest, verifyDefaultProgramming) {
  auto setup = []() {};

  auto verify = [=, this]() {
    auto switchId = getCurrentSwitchIdForTesting();
    auto asicType = hwAsicForSwitch(switchId)->getAsicType();

    std::string out;
    getAgentEnsemble()->runDiagCommand("\n", out, switchId);

    auto golden = loadGoldenData(asicType, querySdkMajorVersion(switchId));
    if (!golden.has_value()) {
      golden = loadGoldenData(asicType, "default");
    }
    ASSERT_TRUE(golden.has_value()) << "No golden data found";

    WITH_RETRIES({
      auto fetched = fetchData(asicType, switchId);
      for (const auto& [key, goldenValue] : *golden) {
        auto fetchedValue = fetched.find(key);
        if (fetchedValue != fetched.end()) {
          EXPECT_EVENTUALLY_EQ(fetchedValue->second, goldenValue)
              << "Diff in key: " << key;
        } else {
          EXPECT_EVENTUALLY_TRUE(false)
              << "Missing key in fetched data: " << key << "=" << goldenValue;
        }
      }
      for (const auto& [key, fetchedValue] : fetched) {
        EXPECT_EVENTUALLY_TRUE(golden->contains(key))
            << "Extra key in fetched data: " << key << " = " << fetchedValue;
      }
    });

    if (!FLAGS_dump_golden_data.empty()) {
      auto dumpData = fetchData(asicType, switchId);
      std::ofstream ofs(FLAGS_dump_golden_data);
      for (const auto& [key, value] : dumpData) {
        std::string type, var;
        folly::split(':', key, type, var);
        ofs << type << "," << var << "," << value << "\n";
      }
    }
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
