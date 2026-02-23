#!/usr/bin/env python3

# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import csv
import os
import tempfile
import unittest

import pandas as pd
from fboss.oss.scripts.oss_test_results import (
    build_scuba_query,
    deduplicate_results,
    filter_results,
    generate_markdown,
    get_csv_output_fields,
    has_null_fields,
    is_null_string,
    parse_test_config,
    sanitize_filename,
    should_skip_config,
    should_skip_test_case,
    SKIP_ASICS,
    SKIP_PLATFORMS,
    TestTypeConfig,
    VENDOR_NAMES,
    write_csvs_for_test_type,
)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _bsp_config() -> TestTypeConfig:
    return TestTypeConfig(
        test_type="BSP Tests",
        scuba_table="netcastle_test_results",
        team="fboss_bsp",
        purpose="DIFF",
        result_fields=["Test Case", "Test Config", "Status"],
    )


def _agent_hw_config() -> TestTypeConfig:
    return TestTypeConfig(
        test_type="Agent HW Test",
        scuba_table="fboss_testing",
        team="sai_agent_test",
        purpose="CONTINUOUS",
        result_fields=["Test Case", "Hardware", "Asic", "Test Config", "Status"],
    )


def _agent_bench_config() -> TestTypeConfig:
    return TestTypeConfig(
        test_type="Agent Benchmarks",
        scuba_table="fboss_testing",
        team="sai_bench",
        purpose="CONTINUOUS",
        result_fields=[
            "Test Case",
            "Hardware",
            "Benchmark Metric Key",
            "Benchmark Metric Value",
            "Test Config",
        ],
    )


def _link_test_config() -> TestTypeConfig:
    return TestTypeConfig(
        test_type="Link Tests",
        scuba_table="fboss_testing",
        team="fboss_link",
        purpose="CONTINUOUS",
        result_fields=["Test Case", "Test Config", "Status"],
    )


def _qsfp_hw_config() -> TestTypeConfig:
    return TestTypeConfig(
        test_type="Qsfp HW Tests",
        scuba_table="fboss_testing",
        team="fboss_qsfp",
        purpose="CONTINUOUS",
        result_fields=["Test Case", "Test Config", "Status"],
    )


def _fsdb_bench_config() -> TestTypeConfig:
    return TestTypeConfig(
        test_type="FSDB Benchmarks",
        scuba_table="fboss_testing",
        team="fsdb_bench",
        purpose="CONTINUOUS",
        result_fields=[
            "Test Case",
            "Hardware",
            "Benchmark Metric Key",
            "Benchmark Metric Value",
            "Test Config",
        ],
    )


def _qsfp_bench_config() -> TestTypeConfig:
    return TestTypeConfig(
        test_type="Qsfp Benchmarks",
        scuba_table="fboss_testing",
        team="qsfp_bench",
        purpose="CONTINUOUS",
        result_fields=[
            "Test Case",
            "Hardware",
            "Benchmark Metric Key",
            "Benchmark Metric Value",
            "Test Config",
        ],
    )


def _fan_config() -> TestTypeConfig:
    return TestTypeConfig(
        test_type="Fan Tests",
        scuba_table="netcastle_test_results",
        team="fboss_fan",
        purpose="CONTINUOUS",
        result_fields=["Test Case", "Test Config", "Status"],
    )


# ---------------------------------------------------------------------------
# Test: should_skip_test_case
# ---------------------------------------------------------------------------


class TestShouldSkipTestCase(unittest.TestCase):
    def test_skip_roundtrip(self) -> None:
        self.assertTrue(should_skip_test_case("HwTest.RoundtripVerify"))

    def test_skip_roundtrip_case_insensitive(self) -> None:
        self.assertTrue(should_skip_test_case("HwTest.roundtrip"))

    def test_skip_preprod(self) -> None:
        self.assertTrue(should_skip_test_case("PreprodSanity.CheckLink"))

    def test_skip_warm_boot_for_warm_boot(self) -> None:
        self.assertTrue(should_skip_test_case("warm_boot_for_warm_boot.HwTest.Foo"))

    def test_no_skip_normal(self) -> None:
        self.assertFalse(should_skip_test_case("HwTest.CheckInit"))

    def test_no_skip_empty(self) -> None:
        self.assertFalse(should_skip_test_case(""))


# ---------------------------------------------------------------------------
# Test: has_null_fields
# ---------------------------------------------------------------------------


class TestHasNullFields(unittest.TestCase):
    def test_no_nulls(self) -> None:
        row = {"a": "val", "b": 42}
        self.assertFalse(has_null_fields(row, ["a", "b"]))

    def test_none_value(self) -> None:
        row = {"a": None, "b": 42}
        self.assertTrue(has_null_fields(row, ["a", "b"]))

    def test_nan_value(self) -> None:
        row = {"a": float("nan"), "b": 42}
        self.assertTrue(has_null_fields(row, ["a", "b"]))

    def test_missing_key(self) -> None:
        row = {"a": "val"}
        self.assertTrue(has_null_fields(row, ["a", "b"]))

    def test_empty_fields(self) -> None:
        row = {"a": "val"}
        self.assertFalse(has_null_fields(row, []))


# ---------------------------------------------------------------------------
# Test: is_null_string
# ---------------------------------------------------------------------------


class TestIsNullString(unittest.TestCase):
    def test_null_lowercase(self) -> None:
        self.assertTrue(is_null_string("null"))

    def test_null_uppercase(self) -> None:
        self.assertTrue(is_null_string("NULL"))

    def test_null_with_whitespace(self) -> None:
        self.assertTrue(is_null_string("  null  "))

    def test_not_null(self) -> None:
        self.assertFalse(is_null_string("meru800bfa"))

    def test_non_string(self) -> None:
        self.assertFalse(is_null_string(42))
        self.assertFalse(is_null_string(None))


# ---------------------------------------------------------------------------
# Test: sanitize_filename
# ---------------------------------------------------------------------------


class TestSanitizeFilename(unittest.TestCase):
    def test_simple(self) -> None:
        self.assertEqual(sanitize_filename("darwin"), "darwin")

    def test_spaces_and_special_chars(self) -> None:
        self.assertEqual(sanitize_filename("Agent HW Test"), "agent_hw_test")

    def test_slashes(self) -> None:
        self.assertEqual(
            sanitize_filename("brcm/11.0/11.0/memory_only/GILLETTE_VOQ"),
            "brcm_11_0_11_0_memory_only_gillette_voq",
        )

    def test_preserves_hyphens(self) -> None:
        self.assertEqual(sanitize_filename("credo-0.7.2"), "credo-0_7_2")


# ---------------------------------------------------------------------------
# Test: build_scuba_query
# ---------------------------------------------------------------------------


class TestBuildScubaQuery(unittest.TestCase):
    def test_contains_table_and_filters(self) -> None:
        config = _bsp_config()
        query = build_scuba_query(config)
        self.assertIn("`netcastle_test_results`", query)
        self.assertIn("`Team` = 'fboss_bsp'", query)
        self.assertIn("`Purpose` = 'DIFF'", query)

    def test_includes_time_column(self) -> None:
        config = _bsp_config()
        query = build_scuba_query(config)
        self.assertIn("`time`", query)

    def test_includes_all_result_fields(self) -> None:
        config = _agent_hw_config()
        query = build_scuba_query(config)
        for field in config.result_fields:
            self.assertIn(f"`{field}`", query)


# ---------------------------------------------------------------------------
# Test: parse_test_config
# ---------------------------------------------------------------------------


class TestParseTestConfig(unittest.TestCase):
    # -- Platform-only types ------------------------------------------------

    def test_bsp_tests(self) -> None:
        result = parse_test_config("BSP Tests", "meru800bfa")
        self.assertEqual(result, {"Platform": "meru800bfa"})

    def test_fan_tests(self) -> None:
        result = parse_test_config("Fan Tests", "darwin")
        self.assertEqual(result, {"Platform": "darwin"})

    def test_led_tests(self) -> None:
        result = parse_test_config("LED Tests", "montblanc")
        self.assertEqual(result, {"Platform": "montblanc"})

    def test_firmware_util(self) -> None:
        result = parse_test_config("Firmware Util", "darwin")
        self.assertEqual(result, {"Platform": "darwin"})

    def test_sensors_tests(self) -> None:
        result = parse_test_config("Sensors Tests", "meru800bia")
        self.assertEqual(result, {"Platform": "meru800bia"})

    def test_data_corral(self) -> None:
        result = parse_test_config("Data Corral Tests", "darwin")
        self.assertEqual(result, {"Platform": "darwin"})

    def test_platform_manager(self) -> None:
        result = parse_test_config("Platform Manager", "meru800bfa")
        self.assertEqual(result, {"Platform": "meru800bfa"})

    # -- Agent HW Test / SAI Test -------------------------------------------

    def test_agent_hw_test_full(self) -> None:
        result = parse_test_config(
            "Agent HW Test",
            "brcm/11.0/11.0/memory_only/GILLETTE_VOQ/fap",
        )
        self.assertEqual(result["Vendor"], "Broadcom")
        self.assertEqual(result["SDK Version"], "11.0")
        self.assertEqual(result["ASIC"], "memory_only")
        self.assertEqual(result["Switch Mode"], "GILLETTE VOQ")
        self.assertEqual(result["Role"], "fap")

    def test_agent_hw_test_no_role(self) -> None:
        result = parse_test_config(
            "Agent HW Test",
            "brcm/11.0/11.0/memory_only/GILLETTE_VOQ",
        )
        self.assertEqual(result["Vendor"], "Broadcom")
        self.assertEqual(result["Role"], "")

    def test_sai_test_leaba_vendor(self) -> None:
        result = parse_test_config(
            "SAI Test",
            "leaba/1.42/1.42/MEMORY_ONLY/MEMORY_GILLETTE_VOQ",
        )
        self.assertEqual(result["Vendor"], "Cisco")
        self.assertEqual(result["SDK Version"], "1.42")
        self.assertEqual(result["ASIC"], "MEMORY_ONLY")

    def test_agent_hw_test_unknown_vendor(self) -> None:
        result = parse_test_config(
            "Agent HW Test",
            "newvendor/1.0/1.0/chipX/MODE",
        )
        self.assertEqual(result["Vendor"], "newvendor")

    # -- Agent Benchmarks ---------------------------------------------------

    def test_agent_benchmarks_full(self) -> None:
        result = parse_test_config(
            "Agent Benchmarks",
            "brcm/11.0/memory_only/GILLETTE_VOQ/fap",
        )
        self.assertEqual(result["Vendor"], "Broadcom")
        self.assertEqual(result["SDK Version"], "11.0")
        self.assertEqual(result["ASIC"], "memory_only")
        self.assertEqual(result["Switch Mode"], "GILLETTE VOQ")
        self.assertEqual(result["Role"], "fap")

    def test_agent_benchmarks_no_role(self) -> None:
        result = parse_test_config(
            "Agent Benchmarks",
            "brcm/11.0/memory_only/GILLETTE_VOQ",
        )
        self.assertEqual(result["Role"], "")

    # -- Link Tests ---------------------------------------------------------

    def test_link_tests(self) -> None:
        result = parse_test_config(
            "Link Tests",
            "darwin/sai/asicsdk-11.0.0.0_odp/11.0_odp",
        )
        self.assertEqual(result["Platform"], "darwin")
        self.assertEqual(result["ASIC SDK"], "11.0.0.0_odp")

    def test_link_tests_no_asicsdk(self) -> None:
        result = parse_test_config("Link Tests", "darwin")
        self.assertEqual(result, {"Platform": "darwin"})

    # -- Qsfp HW Tests -----------------------------------------------------

    def test_qsfp_hw_tests(self) -> None:
        result = parse_test_config(
            "Qsfp HW Tests",
            "darwin_mix/physdk-credo-0.7.2/credo-0.7.2",
        )
        self.assertEqual(result["Platform"], "darwin_mix")
        self.assertEqual(result["PHY SDK"], "credo-0.7.2")

    def test_qsfp_hw_tests_no_phy(self) -> None:
        result = parse_test_config("Qsfp HW Tests", "darwin_mix")
        self.assertEqual(result, {"Platform": "darwin_mix"})

    # -- FSDB Benchmarks ----------------------------------------------------

    def test_fsdb_benchmarks(self) -> None:
        result = parse_test_config(
            "FSDB Benchmarks",
            "fsdb/darwin/11.0/brcm",
        )
        self.assertEqual(result["Component"], "fsdb")
        self.assertEqual(result["Platform"], "darwin")
        self.assertEqual(result["SDK Version"], "11.0")
        self.assertEqual(result["Vendor"], "Broadcom")

    # -- Qsfp Benchmarks ----------------------------------------------------

    def test_qsfp_benchmarks(self) -> None:
        result = parse_test_config(
            "Qsfp Benchmarks",
            "darwin/credo-0.7.2",
        )
        self.assertEqual(result["Platform"], "darwin")
        self.assertEqual(result["PHY SDK"], "credo-0.7.2")

    # -- Fallback -----------------------------------------------------------

    def test_unknown_test_type_fallback(self) -> None:
        result = parse_test_config("Unknown Type", "some/config/string")
        self.assertEqual(result, {"Platform": "some/config/string"})


# ---------------------------------------------------------------------------
# Test: should_skip_config
# ---------------------------------------------------------------------------


class TestShouldSkipConfig(unittest.TestCase):
    def setUp(self) -> None:
        # Save originals and clear for test isolation
        self._orig_platforms = SKIP_PLATFORMS.copy()
        self._orig_asics = SKIP_ASICS.copy()
        SKIP_PLATFORMS.clear()
        SKIP_ASICS.clear()

    def tearDown(self) -> None:
        SKIP_PLATFORMS.clear()
        SKIP_PLATFORMS.update(self._orig_platforms)
        SKIP_ASICS.clear()
        SKIP_ASICS.update(self._orig_asics)

    def test_no_skip_when_lists_empty(self) -> None:
        self.assertFalse(should_skip_config({"Platform": "darwin"}))

    def test_skip_platform_match(self) -> None:
        SKIP_PLATFORMS.add("darwin")
        self.assertTrue(should_skip_config({"Platform": "darwin"}))

    def test_skip_platform_case_insensitive(self) -> None:
        SKIP_PLATFORMS.add("Darwin")
        self.assertTrue(should_skip_config({"Platform": "darwin"}))

    def test_skip_asic_match(self) -> None:
        SKIP_ASICS.add("memory_only")
        self.assertTrue(should_skip_config({"ASIC": "memory_only"}))

    def test_skip_asic_case_insensitive(self) -> None:
        SKIP_ASICS.add("MEMORY_ONLY")
        self.assertTrue(should_skip_config({"ASIC": "memory_only"}))

    def test_no_skip_different_platform(self) -> None:
        SKIP_PLATFORMS.add("darwin")
        self.assertFalse(should_skip_config({"Platform": "montblanc"}))

    def test_no_skip_when_key_missing(self) -> None:
        SKIP_PLATFORMS.add("darwin")
        self.assertFalse(should_skip_config({"Vendor": "Broadcom"}))

    def test_skip_platform_over_asic(self) -> None:
        """Platform match triggers skip even if ASIC doesn't match."""
        SKIP_PLATFORMS.add("darwin")
        self.assertTrue(
            should_skip_config({"Platform": "darwin", "ASIC": "memory_only"})
        )


# ---------------------------------------------------------------------------
# Test: filter_results
# ---------------------------------------------------------------------------


class TestFilterResults(unittest.TestCase):
    def _make_df(self, rows):
        return pd.DataFrame(rows)

    def test_keeps_valid_rows(self) -> None:
        df = self._make_df(
            [
                {
                    "Test Case": "HwTest.Init",
                    "Test Config": "darwin",
                    "Status": "PASSED",
                },
            ]
        )
        result = filter_results(df, _bsp_config())
        self.assertEqual(len(result), 1)

    def test_removes_null_fields(self) -> None:
        df = self._make_df(
            [
                {"Test Case": "HwTest.Init", "Test Config": "darwin", "Status": None},
            ]
        )
        result = filter_results(df, _bsp_config())
        self.assertEqual(len(result), 0)

    def test_removes_nan_fields(self) -> None:
        df = self._make_df(
            [
                {
                    "Test Case": "HwTest.Init",
                    "Test Config": "darwin",
                    "Status": float("nan"),
                },
            ]
        )
        result = filter_results(df, _bsp_config())
        self.assertEqual(len(result), 0)

    def test_removes_null_string_config(self) -> None:
        df = self._make_df(
            [
                {"Test Case": "HwTest.Init", "Test Config": "null", "Status": "PASSED"},
            ]
        )
        result = filter_results(df, _bsp_config())
        self.assertEqual(len(result), 0)

    def test_removes_skip_pattern_match(self) -> None:
        df = self._make_df(
            [
                {
                    "Test Case": "HwTest.RoundtripCheck",
                    "Test Config": "darwin",
                    "Status": "PASSED",
                },
                {
                    "Test Case": "PreprodSanity.Foo",
                    "Test Config": "darwin",
                    "Status": "PASSED",
                },
                {
                    "Test Case": "HwTest.Init",
                    "Test Config": "darwin",
                    "Status": "PASSED",
                },
            ]
        )
        result = filter_results(df, _bsp_config())
        self.assertEqual(len(result), 1)
        self.assertEqual(result.iloc[0]["Test Case"], "HwTest.Init")

    def test_mixed_valid_and_invalid(self) -> None:
        df = self._make_df(
            [
                {"Test Case": "A", "Test Config": "darwin", "Status": "PASSED"},
                {"Test Case": "B", "Test Config": "null", "Status": "PASSED"},
                {"Test Case": "C", "Test Config": "darwin", "Status": None},
                {
                    "Test Case": "Roundtrip.D",
                    "Test Config": "darwin",
                    "Status": "PASSED",
                },
                {"Test Case": "E", "Test Config": "darwin", "Status": "FAILED"},
            ]
        )
        result = filter_results(df, _bsp_config())
        self.assertEqual(len(result), 2)
        self.assertListEqual(list(result["Test Case"]), ["A", "E"])


# ---------------------------------------------------------------------------
# Test: deduplicate_results
# ---------------------------------------------------------------------------


class TestDeduplicateResults(unittest.TestCase):
    def test_keeps_latest(self) -> None:
        df = pd.DataFrame(
            [
                {
                    "Test Case": "A",
                    "Test Config": "darwin",
                    "Status": "FAILED",
                    "time": 100,
                },
                {
                    "Test Case": "A",
                    "Test Config": "darwin",
                    "Status": "PASSED",
                    "time": 200,
                },
            ]
        )
        result = deduplicate_results(df)
        self.assertEqual(len(result), 1)
        self.assertEqual(result.iloc[0]["Status"], "PASSED")

    def test_different_configs_kept(self) -> None:
        df = pd.DataFrame(
            [
                {
                    "Test Case": "A",
                    "Test Config": "darwin",
                    "Status": "PASSED",
                    "time": 100,
                },
                {
                    "Test Case": "A",
                    "Test Config": "montblanc",
                    "Status": "FAILED",
                    "time": 200,
                },
            ]
        )
        result = deduplicate_results(df)
        self.assertEqual(len(result), 2)

    def test_different_test_cases_kept(self) -> None:
        df = pd.DataFrame(
            [
                {
                    "Test Case": "A",
                    "Test Config": "darwin",
                    "Status": "PASSED",
                    "time": 100,
                },
                {
                    "Test Case": "B",
                    "Test Config": "darwin",
                    "Status": "FAILED",
                    "time": 100,
                },
            ]
        )
        result = deduplicate_results(df)
        self.assertEqual(len(result), 2)

    def test_time_column_dropped(self) -> None:
        df = pd.DataFrame(
            [
                {
                    "Test Case": "A",
                    "Test Config": "darwin",
                    "Status": "PASSED",
                    "time": 100,
                },
            ]
        )
        result = deduplicate_results(df)
        self.assertNotIn("time", result.columns)

    def test_multiple_duplicates(self) -> None:
        df = pd.DataFrame(
            [
                {
                    "Test Case": "A",
                    "Test Config": "darwin",
                    "Status": "FAILED",
                    "time": 100,
                },
                {
                    "Test Case": "A",
                    "Test Config": "darwin",
                    "Status": "FAILED",
                    "time": 150,
                },
                {
                    "Test Case": "A",
                    "Test Config": "darwin",
                    "Status": "PASSED",
                    "time": 200,
                },
                {
                    "Test Case": "A",
                    "Test Config": "darwin",
                    "Status": "FAILED",
                    "time": 50,
                },
            ]
        )
        result = deduplicate_results(df)
        self.assertEqual(len(result), 1)
        self.assertEqual(result.iloc[0]["Status"], "PASSED")


# ---------------------------------------------------------------------------
# Test: get_csv_output_fields
# ---------------------------------------------------------------------------


class TestGetCsvOutputFields(unittest.TestCase):
    def test_bsp_replaces_test_config_with_platform(self) -> None:
        fields = get_csv_output_fields(_bsp_config())
        self.assertEqual(fields, ["Test Case", "Platform", "Status"])

    def test_agent_hw_deduplicates_asic(self) -> None:
        """Agent HW Test has Scuba 'Asic' and parsed 'ASIC' — only parsed kept."""
        fields = get_csv_output_fields(_agent_hw_config())
        self.assertIn("ASIC", fields)
        self.assertNotIn("Asic", fields)
        # Verify order: Test Case, Hardware, then parsed columns, then Status
        self.assertEqual(fields[0], "Test Case")
        self.assertEqual(fields[1], "Hardware")
        self.assertEqual(fields[-1], "Status")

    def test_agent_benchmarks_no_status(self) -> None:
        fields = get_csv_output_fields(_agent_bench_config())
        self.assertNotIn("Status", fields)
        self.assertIn("Benchmark Metric Key", fields)

    def test_link_tests(self) -> None:
        fields = get_csv_output_fields(_link_test_config())
        self.assertEqual(fields, ["Test Case", "Platform", "ASIC SDK", "Status"])

    def test_qsfp_hw_tests(self) -> None:
        fields = get_csv_output_fields(_qsfp_hw_config())
        self.assertEqual(fields, ["Test Case", "Platform", "PHY SDK", "Status"])

    def test_fsdb_benchmarks(self) -> None:
        fields = get_csv_output_fields(_fsdb_bench_config())
        self.assertIn("Component", fields)
        self.assertIn("Platform", fields)
        self.assertIn("SDK Version", fields)
        self.assertIn("Vendor", fields)


# ---------------------------------------------------------------------------
# Test: write_csvs_for_test_type
# ---------------------------------------------------------------------------


class TestWriteCsvsForTestType(unittest.TestCase):
    def setUp(self) -> None:
        self.tmpdir = tempfile.mkdtemp()
        self._orig_platforms = SKIP_PLATFORMS.copy()
        self._orig_asics = SKIP_ASICS.copy()
        SKIP_PLATFORMS.clear()
        SKIP_ASICS.clear()

    def tearDown(self) -> None:
        SKIP_PLATFORMS.clear()
        SKIP_PLATFORMS.update(self._orig_platforms)
        SKIP_ASICS.clear()
        SKIP_ASICS.update(self._orig_asics)

    def test_writes_csv_file(self) -> None:
        df = pd.DataFrame(
            [
                {
                    "Test Case": "HwTest.Init",
                    "Test Config": "darwin",
                    "Status": "PASSED",
                },
                {
                    "Test Case": "HwTest.Run",
                    "Test Config": "darwin",
                    "Status": "FAILED",
                },
            ]
        )
        meta = write_csvs_for_test_type(df, _bsp_config(), self.tmpdir)
        self.assertEqual(len(meta), 1)
        self.assertEqual(meta[0]["total"], 2)
        self.assertEqual(meta[0]["passed"], 1)
        self.assertEqual(meta[0]["failed"], 1)
        self.assertEqual(meta[0]["pass_pct"], 50.0)
        # Verify CSV file exists and has correct content
        filepath = os.path.join(self.tmpdir, meta[0]["csv_file"])
        self.assertTrue(os.path.exists(filepath))
        with open(filepath) as f:
            reader = csv.DictReader(f)
            rows = list(reader)
        self.assertEqual(len(rows), 2)
        self.assertEqual(rows[0]["Platform"], "darwin")

    def test_multiple_configs_produce_multiple_csvs(self) -> None:
        df = pd.DataFrame(
            [
                {"Test Case": "A", "Test Config": "darwin", "Status": "PASSED"},
                {"Test Case": "B", "Test Config": "montblanc", "Status": "PASSED"},
            ]
        )
        meta = write_csvs_for_test_type(df, _bsp_config(), self.tmpdir)
        self.assertEqual(len(meta), 2)
        configs = {m["test_config"] for m in meta}
        self.assertEqual(configs, {"darwin", "montblanc"})

    def test_agent_hw_parsed_columns_in_csv(self) -> None:
        df = pd.DataFrame(
            [
                {
                    "Test Case": "HwTest.Init",
                    "Hardware": "wedge100",
                    "Asic": "memory_only",
                    "Test Config": "brcm/11.0/11.0/memory_only/GILLETTE_VOQ/fap",
                    "Status": "PASSED",
                },
            ]
        )
        meta = write_csvs_for_test_type(df, _agent_hw_config(), self.tmpdir)
        filepath = os.path.join(self.tmpdir, meta[0]["csv_file"])
        with open(filepath) as f:
            reader = csv.DictReader(f)
            rows = list(reader)
        self.assertEqual(rows[0]["Vendor"], "Broadcom")
        self.assertEqual(rows[0]["SDK Version"], "11.0")
        self.assertEqual(rows[0]["ASIC"], "memory_only")
        self.assertEqual(rows[0]["Switch Mode"], "GILLETTE VOQ")
        self.assertEqual(rows[0]["Role"], "fap")
        # Scuba "Asic" column should NOT appear (deduped with parsed "ASIC")
        self.assertNotIn("Asic", rows[0])

    def test_benchmark_no_status_fields(self) -> None:
        df = pd.DataFrame(
            [
                {
                    "Test Case": "Bench.Throughput",
                    "Hardware": "wedge100",
                    "Benchmark Metric Key": "throughput_mbps",
                    "Benchmark Metric Value": "1000",
                    "Test Config": "brcm/11.0/memory_only/GILLETTE_VOQ",
                },
            ]
        )
        meta = write_csvs_for_test_type(df, _agent_bench_config(), self.tmpdir)
        self.assertEqual(len(meta), 1)
        self.assertNotIn("passed", meta[0])
        self.assertNotIn("failed", meta[0])

    def test_skip_platform_excludes_config(self) -> None:
        SKIP_PLATFORMS.add("darwin")
        df = pd.DataFrame(
            [
                {"Test Case": "A", "Test Config": "darwin", "Status": "PASSED"},
                {"Test Case": "B", "Test Config": "montblanc", "Status": "PASSED"},
            ]
        )
        meta = write_csvs_for_test_type(df, _bsp_config(), self.tmpdir)
        self.assertEqual(len(meta), 1)
        self.assertEqual(meta[0]["test_config"], "montblanc")

    def test_skip_asic_excludes_config(self) -> None:
        SKIP_ASICS.add("memory_only")
        df = pd.DataFrame(
            [
                {
                    "Test Case": "HwTest.Init",
                    "Hardware": "wedge100",
                    "Asic": "memory_only",
                    "Test Config": "brcm/11.0/11.0/memory_only/GILLETTE_VOQ/fap",
                    "Status": "PASSED",
                },
                {
                    "Test Case": "HwTest.Init",
                    "Hardware": "wedge100",
                    "Asic": "memory_only",
                    "Test Config": "brcm/11.0/11.0/memory_hw/GILLETTE_VOQ/fap",
                    "Status": "PASSED",
                },
            ]
        )
        meta = write_csvs_for_test_type(df, _agent_hw_config(), self.tmpdir)
        self.assertEqual(len(meta), 1)
        self.assertIn("memory_hw", meta[0]["test_config"])

    def test_skip_all_configs_returns_empty(self) -> None:
        SKIP_PLATFORMS.add("darwin")
        df = pd.DataFrame(
            [
                {"Test Case": "A", "Test Config": "darwin", "Status": "PASSED"},
            ]
        )
        meta = write_csvs_for_test_type(df, _bsp_config(), self.tmpdir)
        self.assertEqual(len(meta), 0)

    def test_csv_filename_format(self) -> None:
        df = pd.DataFrame(
            [
                {"Test Case": "A", "Test Config": "darwin", "Status": "PASSED"},
            ]
        )
        meta = write_csvs_for_test_type(df, _fan_config(), self.tmpdir)
        self.assertEqual(meta[0]["csv_file"], "fan_tests_darwin.csv")


# ---------------------------------------------------------------------------
# Test: generate_markdown
# ---------------------------------------------------------------------------


class TestGenerateMarkdown(unittest.TestCase):
    def test_generates_file(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            type_summary = [
                {
                    "test_type": "BSP Tests",
                    "total": 10,
                    "passed": 9,
                    "failed": 1,
                    "pass_pct": 90.0,
                },
            ]
            all_details = [
                {
                    "test_type": "BSP Tests",
                    "test_config": "darwin",
                    "csv_file": "bsp_tests_darwin.csv",
                },
            ]
            generate_markdown(type_summary, all_details, tmpdir)
            filepath = os.path.join(tmpdir, "test_results.mdx")
            self.assertTrue(os.path.exists(filepath))
            with open(filepath) as f:
                content = f.read()
            self.assertIn("FBOSS Test Results", content)
            self.assertIn("BSP Tests", content)

    def test_contains_summary_data(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            type_summary = [
                {
                    "test_type": "Fan Tests",
                    "total": 5,
                    "passed": 5,
                    "failed": 0,
                    "pass_pct": 100.0,
                },
            ]
            generate_markdown(type_summary, [], tmpdir)
            with open(os.path.join(tmpdir, "test_results.mdx")) as f:
                content = f.read()
            self.assertIn('"name": "Fan Tests"', content)
            self.assertIn('"total": 5', content)
            self.assertIn('"passed": 5', content)
            self.assertIn('"pct": 100.0', content)

    def test_contains_details_data(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            all_details = [
                {
                    "test_type": "Fan Tests",
                    "test_config": "darwin",
                    "csv_file": "fan_tests_darwin.csv",
                },
            ]
            generate_markdown([], all_details, tmpdir)
            with open(os.path.join(tmpdir, "test_results.mdx")) as f:
                content = f.read()
            self.assertIn("Fan Tests", content)
            self.assertIn("darwin", content)
            self.assertIn("fan_tests_darwin.csv", content)

    def test_benchmark_no_pass_pct(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            type_summary = [
                {"test_type": "Agent Benchmarks", "total": 10},
            ]
            generate_markdown(type_summary, [], tmpdir)
            with open(os.path.join(tmpdir, "test_results.mdx")) as f:
                content = f.read()
            self.assertIn('"name": "Agent Benchmarks"', content)
            self.assertIn('"total": 10', content)
            self.assertIn('"passed": null', content)
            self.assertIn('"failed": null', content)
            self.assertIn('"pct": null', content)

    def test_contains_docusaurus_frontmatter(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            generate_markdown([], [], tmpdir)
            with open(os.path.join(tmpdir, "test_results.mdx")) as f:
                content = f.read()
            self.assertIn("id: test_results", content)
            self.assertIn("title: FBOSS Test Results", content)
            self.assertIn("oncall: fboss_oss", content)

    def test_imports_react_component(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            generate_markdown([], [], tmpdir)
            with open(os.path.join(tmpdir, "test_results.mdx")) as f:
                content = f.read()
            self.assertIn(
                "import TestResultsDashboard from '@site/src/components/TestResultsDashboard';",
                content,
            )
            self.assertIn("<TestResultsDashboard", content)

    def test_empty_results(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            generate_markdown([], [], tmpdir)
            filepath = os.path.join(tmpdir, "test_results.mdx")
            self.assertTrue(os.path.exists(filepath))


# ---------------------------------------------------------------------------
# Test: end-to-end pipeline (main loop logic)
# ---------------------------------------------------------------------------


class TestEndToEndPipeline(unittest.TestCase):
    """Tests the filter → deduplicate → write → summarize pipeline."""

    def setUp(self) -> None:
        self.tmpdir = tempfile.mkdtemp()
        self._orig_platforms = SKIP_PLATFORMS.copy()
        self._orig_asics = SKIP_ASICS.copy()
        SKIP_PLATFORMS.clear()
        SKIP_ASICS.clear()

    def tearDown(self) -> None:
        SKIP_PLATFORMS.clear()
        SKIP_PLATFORMS.update(self._orig_platforms)
        SKIP_ASICS.clear()
        SKIP_ASICS.update(self._orig_asics)

    def test_full_pipeline_bsp(self) -> None:
        """Simulate the full pipeline for BSP Tests with duplicates and skip patterns."""
        config = _bsp_config()
        df = pd.DataFrame(
            [
                {
                    "Test Case": "KmodTest.LoadKmods",
                    "Test Config": "meru800bfa",
                    "Status": "PASSED",
                    "time": 100,
                },
                {
                    "Test Case": "KmodTest.LoadKmods",
                    "Test Config": "meru800bfa",
                    "Status": "FAILED",
                    "time": 200,
                },
                {
                    "Test Case": "I2CTest.I2CTransactions",
                    "Test Config": "meru800bfa",
                    "Status": "PASSED",
                    "time": 150,
                },
                {
                    "Test Case": "RoundtripTest.Check",
                    "Test Config": "meru800bfa",
                    "Status": "PASSED",
                    "time": 150,
                },
                {
                    "Test Case": "HwmonTest.Sensors",
                    "Test Config": "meru800bfa",
                    "Status": "PASSED",
                    "time": 100,
                },
                {
                    "Test Case": "HwmonTest.Sensors",
                    "Test Config": "darwin",
                    "Status": "PASSED",
                    "time": 100,
                },
                {
                    "Test Case": "NullTest.Null",
                    "Test Config": "null",
                    "Status": "PASSED",
                    "time": 100,
                },
            ]
        )
        # Step 1: filter
        df = filter_results(df, config)
        # RoundtripTest and null config should be removed
        self.assertEqual(len(df), 5)

        # Step 2: deduplicate
        df = deduplicate_results(df)
        # KmodTest.LoadKmods should have 1 entry (latest, time=200, FAILED)
        self.assertEqual(len(df), 4)
        kmod_row = df[df["Test Case"] == "KmodTest.LoadKmods"].iloc[0]
        self.assertEqual(kmod_row["Status"], "FAILED")

        # Step 3: write CSVs
        csv_dir = os.path.join(self.tmpdir, "csv")
        os.makedirs(csv_dir)
        meta = write_csvs_for_test_type(df, config, csv_dir)

        # Should produce 2 CSV files (meru800bfa and darwin)
        self.assertEqual(len(meta), 2)
        configs = {m["test_config"] for m in meta}
        self.assertEqual(configs, {"meru800bfa", "darwin"})

        # meru800bfa: 3 tests, 2 passed, 1 failed
        meru_meta = next(m for m in meta if m["test_config"] == "meru800bfa")
        self.assertEqual(meru_meta["total"], 3)
        self.assertEqual(meru_meta["passed"], 2)
        self.assertEqual(meru_meta["failed"], 1)

        # darwin: 1 test, 1 passed
        darwin_meta = next(m for m in meta if m["test_config"] == "darwin")
        self.assertEqual(darwin_meta["total"], 1)
        self.assertEqual(darwin_meta["passed"], 1)

    def test_full_pipeline_with_skip_platform(self) -> None:
        """Skip a platform and verify it's excluded from output."""
        SKIP_PLATFORMS.add("darwin")
        config = _bsp_config()
        df = pd.DataFrame(
            [
                {
                    "Test Case": "A",
                    "Test Config": "darwin",
                    "Status": "PASSED",
                    "time": 100,
                },
                {
                    "Test Case": "B",
                    "Test Config": "meru800bfa",
                    "Status": "PASSED",
                    "time": 100,
                },
            ]
        )
        df = filter_results(df, config)
        df = deduplicate_results(df)
        csv_dir = os.path.join(self.tmpdir, "csv2")
        os.makedirs(csv_dir)
        meta = write_csvs_for_test_type(df, config, csv_dir)
        self.assertEqual(len(meta), 1)
        self.assertEqual(meta[0]["test_config"], "meru800bfa")

        # Generate markdown and verify darwin is not in details
        docs_dir = os.path.join(self.tmpdir, "docs")
        generate_markdown([], meta, docs_dir)
        with open(os.path.join(docs_dir, "test_results.mdx")) as f:
            content = f.read()
        self.assertNotIn("darwin", content)
        self.assertIn("meru800bfa", content)

    def test_full_pipeline_agent_hw_with_roles(self) -> None:
        """Agent HW Test with roles parses correctly through the pipeline."""
        config = _agent_hw_config()
        df = pd.DataFrame(
            [
                {
                    "Test Case": "HwTest.Init",
                    "Hardware": "wedge100",
                    "Asic": "memory_only",
                    "Test Config": "brcm/11.0/11.0/memory_only/GILLETTE_VOQ/fap",
                    "Status": "PASSED",
                    "time": 100,
                },
                {
                    "Test Case": "HwTest.Init",
                    "Hardware": "wedge100",
                    "Asic": "memory_only",
                    "Test Config": "brcm/11.0/11.0/memory_only/GILLETTE_VOQ/fe13",
                    "Status": "FAILED",
                    "time": 100,
                },
            ]
        )
        df = filter_results(df, config)
        df = deduplicate_results(df)
        csv_dir = os.path.join(self.tmpdir, "csv3")
        os.makedirs(csv_dir)
        meta = write_csvs_for_test_type(df, config, csv_dir)
        self.assertEqual(len(meta), 2)

        # Verify CSV content for fap role
        fap_meta = next(m for m in meta if "fap" in m["test_config"])
        filepath = os.path.join(csv_dir, fap_meta["csv_file"])
        with open(filepath) as f:
            reader = csv.DictReader(f)
            rows = list(reader)
        self.assertEqual(rows[0]["Role"], "fap")
        self.assertEqual(rows[0]["Vendor"], "Broadcom")


# ---------------------------------------------------------------------------
# Test: vendor name mapping
# ---------------------------------------------------------------------------


class TestVendorNames(unittest.TestCase):
    def test_all_known_vendors(self) -> None:
        self.assertEqual(VENDOR_NAMES["brcm"], "Broadcom")
        self.assertEqual(VENDOR_NAMES["leaba"], "Cisco")
        self.assertEqual(VENDOR_NAMES["nvda"], "Nvidia")
        self.assertEqual(VENDOR_NAMES["chenab"], "Nvidia")

    def test_unknown_vendor_passthrough(self) -> None:
        result = parse_test_config("Agent HW Test", "newvendor/1.0/1.0/chip/MODE")
        self.assertEqual(result["Vendor"], "newvendor")


# ---------------------------------------------------------------------------
# Test: config completeness
# ---------------------------------------------------------------------------


class TestConfigCompleteness(unittest.TestCase):
    """Verify all 14 test types are configured correctly."""

    def test_all_test_types_have_config_columns(self) -> None:
        from fboss.oss.scripts.oss_test_results import CONFIG_COLUMNS, TEST_TYPE_CONFIGS

        for config in TEST_TYPE_CONFIGS:
            self.assertIn(
                config.test_type,
                CONFIG_COLUMNS,
                f"Missing CONFIG_COLUMNS entry for {config.test_type}",
            )

    def test_14_test_types_defined(self) -> None:
        from fboss.oss.scripts.oss_test_results import TEST_TYPE_CONFIGS

        self.assertEqual(len(TEST_TYPE_CONFIGS), 14)

    def test_all_configs_have_test_config_field(self) -> None:
        from fboss.oss.scripts.oss_test_results import TEST_TYPE_CONFIGS

        for config in TEST_TYPE_CONFIGS:
            self.assertIn(
                "Test Config",
                config.result_fields,
                f"{config.test_type} missing 'Test Config' in result_fields",
            )

    def test_all_configs_have_test_case_field(self) -> None:
        from fboss.oss.scripts.oss_test_results import TEST_TYPE_CONFIGS

        for config in TEST_TYPE_CONFIGS:
            self.assertIn(
                "Test Case",
                config.result_fields,
                f"{config.test_type} missing 'Test Case' in result_fields",
            )
