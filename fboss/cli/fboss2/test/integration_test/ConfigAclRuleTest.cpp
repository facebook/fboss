// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for the `acl rule` CLI surface — both
 *   `fboss2-dev config acl rule <table> <rule> <attr> <value>` (setter), and
 *   `fboss2-dev delete acl rule <table> <rule>`               (whole rule).
 *
 * Set tests cover all 16 AclEntry match-field attributes:
 *   source-ip, destination-ip, protocol, source-port, destination-port,
 *   dscp, tcp-flags, icmp-type, icmp-code, ip-fragment, ttl,
 *   destination-mac, ethertype, vlan, ip-type, packet-lookup-result.
 *
 * For each attribute a `Set` test:
 *   `config acl rule` upserts an entry with the attribute set, commit, then
 *   verify the entry's running-config representation contains the field.
 *
 * The `DeleteRule` test exercises whole-entry deletion: create an entry via
 * `config acl rule`, commit, verify present, then `delete acl rule` and
 * verify the entry is gone from the table.
 *
 * SetUpTestSuite unions the table's `qualifiers` list with every
 * AclTableQualifier the tests will exercise and restarts the agent — SAI
 * rejects entry creation with SAI_STATUS_ITEM_NOT_FOUND if the underlying
 * TCAM table wasn't declared to qualify on the relevant fields. Snapshot and
 * restore of /etc/coop/agent.conf is intentionally NOT done here: the test
 * orchestrator (run_test.py's Fboss2IntegrationTestRunner) snapshots the
 * config before the suite and restores it after, so the gtest only needs to
 * widen the table's qualifiers and leave restoration to the harness.
 *
 * Per-test TearDown best-effort deletes the test rule so each test starts
 * from a clean table. The rule itself is created on demand by `config acl
 * rule` (which upserts), so no per-test pre-injection is needed.
 */

#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;
using ::testing::HasSubstr;

namespace {
constexpr auto kTestRuleName = "test-rule-acl-cli";
constexpr auto kInjectedConfPath = "/tmp/agent.conf.injected";

// `/etc/coop/agent.conf` is root-owned. Read it via `sudo cat` instead of
// fopen() so the test process doesn't need to run as root itself.
std::string sudoReadFile(const std::string& path) {
  const std::string cmd = "sudo cat " + path;
  // NOLINTNEXTLINE(concurrency-mt-unsafe,bugprone-unsafe-functions)
  std::FILE* p = ::popen(cmd.c_str(), "r");
  if (!p) {
    throw std::runtime_error("popen failed for: " + cmd);
  }
  std::array<char, 4096> buf{};
  std::string out;
  size_t n = 0;
  while ((n = std::fread(buf.data(), 1, buf.size(), p)) > 0) {
    out.append(buf.data(), n);
  }
  // A read error returns 0 just like EOF; without this check a truncated
  // config would be silently parsed as valid JSON and injected.
  if (std::ferror(p)) {
    ::pclose(p);
    throw std::runtime_error("read error on pipe for: " + cmd);
  }
  int rc = ::pclose(p);
  if (rc != 0) {
    throw std::runtime_error(cmd + " exited with status " + std::to_string(rc));
  }
  return out;
}

// Qualifier IDs from cfg::AclTableQualifier in switch_config.thrift. These
// are the qualifiers the suite's per-attribute tests will exercise — every
// match field that any test sets must be declared on the table, otherwise
// the SAI ACL backend rejects the entry with SAI_STATUS_ITEM_NOT_FOUND.
const std::vector<int64_t>& requiredQualifiers() {
  static const std::vector<int64_t> kIds = {
      0, // SRC_IPV6
      1, // DST_IPV6
      2, // SRC_IPV4
      3, // DST_IPV4
      4, // L4_SRC_PORT
      5, // L4_DST_PORT
      6, // IP_PROTOCOL_NUMBER
      7, // TCP_FLAGS
      10, // IP_FRAG
      11, // ICMPV4_TYPE
      12, // ICMPV4_CODE
      13, // ICMPV6_TYPE
      14, // ICMPV6_CODE
      15, // DSCP
      16, // DST_MAC
      17, // IP_TYPE
      18, // TTL
      22, // ETHER_TYPE
      23, // OUTER_VLAN
  };
  return kIds;
}
} // namespace

class ConfigAclRuleTest : public Fboss2IntegrationTest {
 public:
  // Union the first AclTable's `qualifiers` list with every
  // AclTableQualifier the tests will exercise, then restart fboss_sw_agent
  // so the table is ready for entry creation. The test entry itself is
  // created on demand by `config acl rule` (which upserts), so we don't
  // pre-inject it here.
  //
  // Snapshot/restore of /etc/coop/agent.conf is deliberately omitted: the
  // run_test.py orchestrator (Fboss2IntegrationTestRunner) snapshots the
  // config before the run and restores it after, so this suite only widens
  // the table's qualifiers and leaves restoration to the harness.
  //
  // NOLINTBEGIN(concurrency-mt-unsafe,bugprone-unsafe-functions): suite
  // setup runs single-threaded; std::system is intentional for the
  // sudo cp / systemctl pattern used across the integration suite.
  static void SetUpTestSuite() {
    // The suite mutates /etc/coop/agent.conf and bounces fboss_sw_agent
    // via `sudo cp` / `sudo systemctl restart`. Environments without
    // `sudo` cannot run this; skip the entire suite cleanly rather than
    // failing SetUpTestSuite, which would otherwise disrupt subsequent
    // integration test suites that share the same DUT.
    // NOLINTNEXTLINE(concurrency-mt-unsafe,bugprone-unsafe-functions)
    if (std::system("command -v sudo >/dev/null 2>&1") != 0) {
      GTEST_SKIP() << "sudo not available — ConfigAclRuleTest requires a "
                      "real DUT with sudo + systemd-managed fboss_sw_agent";
    }
    XLOG(INFO) << "[SetUpTestSuite] Adding required qualifiers to AclTable[0]";
    auto cfg = folly::parseJson(sudoReadFile("/etc/coop/agent.conf"));
    // Guard the path access: folly::dynamic operator[] throws on a missing
    // key or out-of-range index. A DUT with no field-56 aclTableGroups (or an
    // empty table list) should skip cleanly rather than abort SetUpTestSuite
    // with an opaque std::out_of_range that fails every test in the suite.
    auto* sw = cfg.get_ptr("sw");
    auto* groups = sw ? sw->get_ptr("aclTableGroups") : nullptr;
    if (!groups || !groups->isArray() || groups->empty() ||
        !(*groups)[0].count("aclTables") ||
        !(*groups)[0]["aclTables"].isArray() ||
        (*groups)[0]["aclTables"].empty()) {
      GTEST_SKIP() << "DUT config has no field-56 aclTableGroups with an "
                      "AclTable — nothing for ConfigAclRuleTest to annotate";
    }
    auto& table = cfg["sw"]["aclTableGroups"][0]["aclTables"][0];

    if (!table.count("qualifiers") || !table["qualifiers"].isArray()) {
      table["qualifiers"] = folly::dynamic::array;
    }
    std::set<int64_t> qualifiers;
    for (const auto& q : table["qualifiers"]) {
      qualifiers.insert(q.asInt());
    }
    for (auto q : requiredQualifiers()) {
      qualifiers.insert(q);
    }
    folly::dynamic mergedQualifiers = folly::dynamic::array;
    for (auto q : qualifiers) {
      mergedQualifiers.push_back(q);
    }
    table["qualifiers"] = std::move(mergedQualifiers);

    {
      std::ofstream out(kInjectedConfPath);
      ASSERT_TRUE(out.is_open()) << "open " << kInjectedConfPath;
      out << folly::toPrettyJson(cfg);
      ASSERT_TRUE(out.good()) << "write " << kInjectedConfPath;
    }
    ASSERT_EQ(
        std::system(("sudo cp " + std::string(kInjectedConfPath) +
                     " /etc/coop/agent.conf")
                        .c_str()),
        0)
        << "inject cp failed";

    XLOG(INFO) << "[SetUpTestSuite] Restarting fboss_sw_agent";
    ASSERT_EQ(std::system("sudo systemctl restart fboss_sw_agent"), 0)
        << "restart failed";
    waitForAgentReadyViaSystemd();
    XLOG(INFO) << "[SetUpTestSuite] Agent back up; suite ready";
  }
  // NOLINTEND(concurrency-mt-unsafe,bugprone-unsafe-functions)

 protected:
  void TearDown() override {
    // Best-effort wipe so each test starts from a clean table. runCli reports
    // failure via exitCode (it does not throw), so inspect it explicitly.
    // targetTable() can still throw if the running config has no ACL tables;
    // swallow that so the framework's TearDown still runs.
    try {
      auto tableName = targetTable();
      auto del = runCli({"delete", "acl", "rule", tableName, kTestRuleName});
      if (del.exitCode == 0) {
        // The rule existed and is staged for deletion; the commit must land,
        // otherwise the next test (which reuses kTestRuleName) would see a
        // stale rule. Surface a commit failure rather than swallowing it.
        auto commit = runCli({"config", "session", "commit"});
        EXPECT_EQ(commit.exitCode, 0)
            << "TearDown commit of rule deletion failed: " << commit.stderr;
      } else {
        // Rule absent (e.g. the test errored before creating it) — nothing to
        // commit. The base SetUp discards any stale session before the next
        // test, so no cleanup is needed here.
        XLOG(INFO) << "TearDown: no '" << kTestRuleName
                   << "' rule to delete (exit " << del.exitCode << ")";
      }
    } catch (const std::exception& e) {
      XLOG(WARN) << "TearDown cleanup skipped: " << e.what();
    }
    Fboss2IntegrationTest::TearDown();
  }

  // The first AclTable of the first AclTableGroup — the one
  // SetUpTestSuite annotated with the required qualifiers. The CLI scans
  // every group looking for a table with this name, so the group index
  // does not need to be conveyed.
  std::string targetTable() const {
    auto config = getRunningConfig();
    const auto& groups = config["sw"]["aclTableGroups"];
    if (groups.empty()) {
      throw std::runtime_error("no aclTableGroups in running config");
    }
    const auto& tables = groups[0]["aclTables"];
    if (tables.empty()) {
      throw std::runtime_error("no aclTables in aclTableGroups[0]");
    }
    return tables[0]["name"].asString();
  }

  // Find the MatchAction attached to a rule via
  // dataPlaneTrafficPolicy.matchToAction. Returns nullopt if none exists.
  std::optional<folly::dynamic> getMatchAction(
      const std::string& ruleName) const {
    auto config = getRunningConfig();
    const auto& sw = config["sw"];
    if (!sw.count("dataPlaneTrafficPolicy")) {
      return std::nullopt;
    }
    const auto& policy = sw["dataPlaneTrafficPolicy"];
    if (!policy.isObject() || !policy.count("matchToAction")) {
      return std::nullopt;
    }
    for (const auto& mta : policy["matchToAction"]) {
      if (mta["matcher"].asString() == ruleName) {
        return mta["action"];
      }
    }
    return std::nullopt;
  }

  std::optional<folly::dynamic> getRule(
      const std::string& tableName,
      const std::string& ruleName) const {
    auto config = getRunningConfig();
    const auto& groups = config["sw"]["aclTableGroups"];
    for (const auto& group : groups) {
      if (!group.count("aclTables")) {
        continue;
      }
      for (const auto& table : group["aclTables"]) {
        if (table["name"].asString() != tableName) {
          continue;
        }
        for (const auto& entry : table["aclEntries"]) {
          if (entry["name"].asString() == ruleName) {
            return entry;
          }
        }
      }
    }
    return std::nullopt;
  }

  void runConfigCli(
      const std::string& table,
      const std::string& rule,
      const std::string& attr,
      const std::vector<std::string>& values) {
    std::vector<std::string> argv{"config", "acl", "rule", table, rule, attr};
    for (const auto& v : values) {
      argv.push_back(v);
    }
    auto result = runCli(argv);
    ASSERT_EQ(result.exitCode, 0)
        << "stdout=" << result.stdout << " stderr=" << result.stderr;
    EXPECT_THAT(result.stdout, HasSubstr(attr));
  }

  void runDeleteCli(const std::string& table, const std::string& rule) {
    auto result = runCli({"delete", "acl", "rule", table, rule});
    ASSERT_EQ(result.exitCode, 0)
        << "stdout=" << result.stdout << " stderr=" << result.stderr;
    EXPECT_THAT(result.stdout, HasSubstr(rule));
  }

  // A single (attribute, values) baseline pair the test sets up before
  // exercising the attribute under test. Some attributes are only valid
  // alongside a matching `protocol` (e.g. icmp-type requires
  // protocol=icmp; tcp-flags requires protocol=tcp), and SAI rejects
  // entries with an inconsistent combination — so the test has to
  // populate the right baseline before mutating the attribute being
  // tested. The baseline is staged via `config acl rule` and committed
  // as a single hitless transaction together with the test attribute.
  struct Preset {
    std::string attr;
    std::vector<std::string> values{};
  };

  void stagePresets(
      const std::string& tableName,
      const std::string& ruleName,
      const std::vector<Preset>& presets) {
    for (const auto& p : presets) {
      runConfigCli(tableName, ruleName, p.attr, p.values);
    }
  }

  // Common driver for a single-attribute `set` test: stage any required
  // baseline, set the value, commit once, verify the running config
  // reflects it.
  template <typename Verify>
  void runSet(
      const std::string& attr,
      const std::vector<std::string>& values,
      Verify verify,
      const std::vector<Preset>& presets = {}) {
    auto tableName = targetTable();
    XLOG(INFO) << "[set] " << attr << " on " << tableName << "/"
               << kTestRuleName;
    stagePresets(tableName, kTestRuleName, presets);
    runConfigCli(tableName, kTestRuleName, attr, values);
    commitConfig();
    auto updated = getRule(tableName, kTestRuleName);
    ASSERT_TRUE(updated.has_value());
    verify(*updated);
  }

  // Driver for `action <subattr> [extras]` tests. The rule is created on
  // demand by setting a benign match-field (dscp) first, then the action
  // is layered on. Both stage operations and the final commit travel as
  // a single hitless transaction.
  template <typename Verify>
  void runActionSet(
      const std::string& subattr,
      const std::vector<std::string>& extras,
      Verify verify) {
    auto tableName = targetTable();
    XLOG(INFO) << "[set-action] " << subattr << " on " << tableName << "/"
               << kTestRuleName;
    runConfigCli(tableName, kTestRuleName, "dscp", {"10"});
    std::vector<std::string> values{subattr};
    for (const auto& e : extras) {
      values.push_back(e);
    }
    runConfigCli(tableName, kTestRuleName, "action", values);
    commitConfig();
    verify(tableName);
  }
};

// =============================================================
// Set tests — `config acl rule <table> <rule> <attr> <value>`
// =============================================================

TEST_F(ConfigAclRuleTest, SetSourceIp) {
  runSet("source-ip", {"10.250.0.0/24"}, [](auto& e) {
    EXPECT_EQ(e["srcIp"].asString(), "10.250.0.0/24");
  });
}

TEST_F(ConfigAclRuleTest, SetDestinationIp) {
  runSet("destination-ip", {"fc00::/64"}, [](auto& e) {
    EXPECT_EQ(e["dstIp"].asString(), "fc00::/64");
  });
}

TEST_F(ConfigAclRuleTest, SetProtocol) {
  runSet(
      "protocol", {"udp"}, [](auto& e) { EXPECT_EQ(e["proto"].asInt(), 17); });
}

TEST_F(ConfigAclRuleTest, SetSourcePort) {
  runSet(
      "source-port",
      {"12345"},
      [](auto& e) { EXPECT_EQ(e["l4SrcPort"].asInt(), 12345); },
      {{"protocol", {"tcp"}}});
}

TEST_F(ConfigAclRuleTest, SetDestinationPort) {
  runSet(
      "destination-port",
      {"443"},
      [](auto& e) { EXPECT_EQ(e["l4DstPort"].asInt(), 443); },
      {{"protocol", {"tcp"}}});
}

TEST_F(ConfigAclRuleTest, SetDscp) {
  runSet("dscp", {"46"}, [](auto& e) { EXPECT_EQ(e["dscp"].asInt(), 46); });
}

TEST_F(ConfigAclRuleTest, SetTcpFlags) {
  runSet(
      "tcp-flags",
      {"0x12"},
      [](auto& e) { EXPECT_EQ(e["tcpFlagsBitMap"].asInt(), 0x12); },
      {{"protocol", {"tcp"}}});
}

TEST_F(ConfigAclRuleTest, SetIcmpType) {
  runSet(
      "icmp-type",
      {"8"},
      [](auto& e) { EXPECT_EQ(e["icmpType"].asInt(), 8); },
      {{"protocol", {"icmp"}}});
}

TEST_F(ConfigAclRuleTest, SetIcmpCode) {
  // icmp-code only makes sense alongside icmp-type (echo-reply / etc.) —
  // SAI rejects an entry that has icmp-code without icmp-type.
  runSet(
      "icmp-code",
      {"0"},
      [](auto& e) { EXPECT_EQ(e["icmpCode"].asInt(), 0); },
      {{"protocol", {"icmp"}}, {"icmp-type", {"8"}}});
}

TEST_F(ConfigAclRuleTest, SetIpFragment) {
  // cfg::IpFragMatch::MATCH_ANY_FRAGMENT = 4
  runSet("ip-fragment", {"any-fragment"}, [](auto& e) {
    EXPECT_EQ(e["ipFrag"].asInt(), 4);
  });
}

TEST_F(ConfigAclRuleTest, SetTtlDefaultMask) {
  runSet("ttl", {"64"}, [](auto& e) {
    EXPECT_EQ(e["ttl"]["value"].asInt(), 64);
    EXPECT_EQ(e["ttl"]["mask"].asInt(), 0xFF);
  });
}

TEST_F(ConfigAclRuleTest, SetTtlExplicitMask) {
  runSet("ttl", {"64", "240"}, [](auto& e) {
    EXPECT_EQ(e["ttl"]["value"].asInt(), 64);
    EXPECT_EQ(e["ttl"]["mask"].asInt(), 240);
  });
}

TEST_F(ConfigAclRuleTest, SetDestinationMac) {
  runSet("destination-mac", {"aa:bb:cc:dd:ee:ff"}, [](auto& e) {
    EXPECT_EQ(e["dstMac"].asString(), "aa:bb:cc:dd:ee:ff");
  });
}

TEST_F(ConfigAclRuleTest, SetEtherType) {
  // EtherType::IPv4 = 0x0800
  runSet("ethertype", {"ipv4"}, [](auto& e) {
    EXPECT_EQ(e["etherType"].asInt(), 0x0800);
  });
}

TEST_F(ConfigAclRuleTest, SetVlan) {
  runSet("vlan", {"100"}, [](auto& e) { EXPECT_EQ(e["vlanID"].asInt(), 100); });
}

TEST_F(ConfigAclRuleTest, SetIpType) {
  // cfg::IpType::IP6 = 3
  runSet(
      "ip-type", {"ipv6"}, [](auto& e) { EXPECT_EQ(e["ipType"].asInt(), 3); });
}

TEST_F(ConfigAclRuleTest, SetPacketLookupResult) {
  runSet("packet-lookup-result", {"mpls-no-match"}, [](auto& e) {
    EXPECT_EQ(e["packetLookupResult"].asInt(), 1);
  });
}

// =============================================================
// Action tests — `config acl rule <table> <rule> action <subattr> [val]`
//
// permit/deny mutate AclEntry.actionType in place. The remaining action
// sub-attrs land on a MatchAction in dataPlaneTrafficPolicy.matchToAction
// keyed by rule name. Each test creates the entry on demand by setting a
// neutral match field (dscp), then exercises the action.
// =============================================================

TEST_F(ConfigAclRuleTest, SetActionPermit) {
  runActionSet("permit", {}, [this](const std::string& table) {
    auto e = getRule(table, kTestRuleName);
    ASSERT_TRUE(e.has_value());
    // PERMIT == 1
    EXPECT_EQ((*e)["actionType"].asInt(), 1);
  });
}

TEST_F(ConfigAclRuleTest, SetActionDeny) {
  runActionSet("deny", {}, [this](const std::string& table) {
    auto e = getRule(table, kTestRuleName);
    ASSERT_TRUE(e.has_value());
    // DENY == 0
    EXPECT_EQ((*e)["actionType"].asInt(), 0);
  });
}

TEST_F(ConfigAclRuleTest, SetActionSendToQueue) {
  runActionSet("send-to-queue", {"3"}, [this](const std::string&) {
    auto a = getMatchAction(kTestRuleName);
    ASSERT_TRUE(a.has_value());
    EXPECT_EQ((*a)["sendToQueue"]["queueId"].asInt(), 3);
  });
}

TEST_F(ConfigAclRuleTest, SetActionSetDscp) {
  runActionSet("set-dscp", {"46"}, [this](const std::string&) {
    auto a = getMatchAction(kTestRuleName);
    ASSERT_TRUE(a.has_value());
    EXPECT_EQ((*a)["setDscp"]["dscpValue"].asInt(), 46);
  });
}

TEST_F(ConfigAclRuleTest, SetActionSetTc) {
  runActionSet("set-tc", {"5"}, [this](const std::string&) {
    auto a = getMatchAction(kTestRuleName);
    ASSERT_TRUE(a.has_value());
    EXPECT_EQ((*a)["setTc"]["tcValue"].asInt(), 5);
  });
}

TEST_F(ConfigAclRuleTest, SetActionTrapToCpu) {
  runActionSet("trap-to-cpu", {}, [this](const std::string&) {
    auto a = getMatchAction(kTestRuleName);
    ASSERT_TRUE(a.has_value());
    // ToCpuAction::TRAP == 1
    EXPECT_EQ((*a)["toCpuAction"].asInt(), 1);
  });
}

TEST_F(ConfigAclRuleTest, SetActionCopyToCpu) {
  runActionSet("copy-to-cpu", {}, [this](const std::string&) {
    auto a = getMatchAction(kTestRuleName);
    ASSERT_TRUE(a.has_value());
    // ToCpuAction::COPY == 0
    EXPECT_EQ((*a)["toCpuAction"].asInt(), 0);
  });
}

// mirror-ingress/mirror-egress and redirect nexthop are intentionally
// covered only at the unit-test layer for now: SAI rejects acl entries
// that reference an undefined mirror name or an unresolvable nexthop,
// and provisioning a real mirror session / nexthop on the DUT is out of
// scope for this PR. The CLI does construct the right config delta
// (proven in the unit tests); end-to-end validation is deferred until a
// follow-up sets up the supporting state.

// =============================================================
// Delete test — `delete acl rule <table> <rule>`
// =============================================================

TEST_F(ConfigAclRuleTest, DeleteRule) {
  auto tableName = targetTable();
  XLOG(INFO) << "[delete-rule] " << tableName << "/" << kTestRuleName;

  runConfigCli(tableName, kTestRuleName, "dscp", {"46"});
  commitConfig();
  ASSERT_TRUE(getRule(tableName, kTestRuleName).has_value())
      << "rule missing after upsert";

  runDeleteCli(tableName, kTestRuleName);
  commitConfig();
  EXPECT_FALSE(getRule(tableName, kTestRuleName).has_value())
      << "rule still present after delete";
}

// Delete must also drop the MatchToAction attached via
// dataPlaneTrafficPolicy.matchToAction. Without that cleanup the second
// commit below would fail with "Invalid config: No acl named X found"
// (ApplyThriftConfig.checkTrafficPolicyAclsExistInConfig).
TEST_F(ConfigAclRuleTest, DeleteRuleWithAction) {
  auto tableName = targetTable();
  XLOG(INFO) << "[delete-rule-with-action] " << tableName << "/"
             << kTestRuleName;

  runConfigCli(tableName, kTestRuleName, "dscp", {"46"});
  runConfigCli(tableName, kTestRuleName, "action", {"set-dscp", "32"});
  commitConfig();
  ASSERT_TRUE(getRule(tableName, kTestRuleName).has_value())
      << "rule missing after upsert";
  ASSERT_TRUE(getMatchAction(kTestRuleName).has_value())
      << "matchToAction missing after action set";

  runDeleteCli(tableName, kTestRuleName);
  commitConfig();
  EXPECT_FALSE(getRule(tableName, kTestRuleName).has_value())
      << "rule still present after delete";
  EXPECT_FALSE(getMatchAction(kTestRuleName).has_value())
      << "matchToAction still present after rule delete";
}
