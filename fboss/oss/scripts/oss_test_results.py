#!/usr/bin/env python3

# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

"""
Query FBOSS test results from Scuba and export as CSVs for the OSS wiki.

Usage:
    buck run fbcode//fboss/oss/scripts:oss_test_results
    buck run fbcode//fboss/oss/scripts:oss_test_results -- --csv_output_dir /tmp/test_results --docs_output_dir /tmp/test_results_docs
"""

import argparse
import csv
import logging
import os
import re
import time
from dataclasses import dataclass
from typing import Any, Callable, Dict, List, Optional

import pandas as pd
from analytics.bamboo import Bamboo as bb


logger = logging.getLogger(__name__)
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
)

# Default output directories
DEFAULT_CSV_OUTPUT_DIR = os.path.join(
    os.path.dirname(__file__),
    "../../github/docs/static/test_results",
)
DEFAULT_DOCS_OUTPUT_DIR = os.path.join(
    os.path.dirname(__file__),
    "../../github/docs/docs/test_results",
)

# Skip rules: test case names containing any of these strings are excluded
SKIP_PATTERNS = ["Roundtrip", "Preprod", "warm_boot_for_warm_boot"]

# Skip rules for test configs: configs whose parsed Platform or ASIC matches
# any entry in these sets are excluded from output (case-insensitive).
# Add platform or ASIC names here to suppress them from the OSS wiki.
SKIP_PLATFORMS: set = set()
SKIP_ASICS: set = set()

# Scuba query lookback in seconds (1 day)
QUERY_LOOKBACK_SECONDS = 24 * 60 * 60

# Vendor short names to display names
VENDOR_NAMES: Dict[str, str] = {
    "brcm": "Broadcom",
    "leaba": "Cisco",
    "nvda": "Nvidia",
    "chenab": "Nvidia",
}

# Parsed config columns that replace "Test Config" in CSV output, per test type.
# Order matters — columns appear in this order in the CSV.
CONFIG_COLUMNS: Dict[str, List[str]] = {
    "Agent HW Test": ["Vendor", "SDK Version", "ASIC", "Switch Mode", "Role"],
    "SAI Test": ["Vendor", "SDK Version", "ASIC", "Switch Mode", "Role"],
    "Agent Benchmarks": ["Vendor", "SDK Version", "ASIC", "Switch Mode", "Role"],
    "BSP Tests": ["Platform"],
    "LED Tests": ["Platform"],
    "Link Tests": ["Platform", "ASIC SDK"],
    "Platform Manager": ["Platform"],
    "Qsfp HW Tests": ["Platform", "PHY SDK"],
    "Fan Tests": ["Platform"],
    "Sensors Tests": ["Platform"],
    "Data Corral Tests": ["Platform"],
    "FSDB Benchmarks": ["Component", "Platform", "SDK Version", "Vendor"],
    "Qsfp Benchmarks": ["Platform", "PHY SDK"],
    "Firmware Util": ["Platform"],
}


@dataclass
class TestTypeConfig:
    """Configuration for a single test type."""

    test_type: str
    scuba_table: str
    team: str
    purpose: str
    result_fields: List[str]


# Map of all 14 test types per the spec
TEST_TYPE_CONFIGS: List[TestTypeConfig] = [
    TestTypeConfig(
        test_type="Agent HW Test",
        scuba_table="fboss_testing",
        team="sai_agent_test",
        purpose="CONTINUOUS",
        result_fields=["Test Case", "Hardware", "Asic", "Test Config", "Status"],
    ),
    TestTypeConfig(
        test_type="SAI Test",
        scuba_table="fboss_testing",
        team="sai_agent_test",
        purpose="CONTINUOUS",
        result_fields=["Test Case", "Hardware", "Asic", "Test Config", "Status"],
    ),
    TestTypeConfig(
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
    ),
    TestTypeConfig(
        test_type="BSP Tests",
        scuba_table="netcastle_test_results",
        team="fboss_bsp",
        purpose="DIFF",
        result_fields=["Test Case", "Test Config", "Status"],
    ),
    TestTypeConfig(
        test_type="LED Tests",
        scuba_table="fboss_testing",
        team="fboss_led",
        purpose="CONTINUOUS",
        result_fields=["Test Case", "Test Config", "Status"],
    ),
    TestTypeConfig(
        test_type="Link Tests",
        scuba_table="fboss_testing",
        team="fboss_link",
        purpose="CONTINUOUS",
        result_fields=["Test Case", "Test Config", "Status"],
    ),
    TestTypeConfig(
        test_type="Platform Manager",
        scuba_table="netcastle_test_results",
        team="fboss_platform_manager",
        purpose="CONTINUOUS",
        result_fields=["Test Case", "Test Config", "Status"],
    ),
    TestTypeConfig(
        test_type="Qsfp HW Tests",
        scuba_table="fboss_testing",
        team="fboss_qsfp",
        purpose="CONTINUOUS",
        result_fields=["Test Case", "Test Config", "Status"],
    ),
    TestTypeConfig(
        test_type="Fan Tests",
        scuba_table="netcastle_test_results",
        team="fboss_fan",
        purpose="CONTINUOUS",
        result_fields=["Test Case", "Test Config", "Status"],
    ),
    TestTypeConfig(
        test_type="Sensors Tests",
        scuba_table="netcastle_test_results",
        team="fboss_sensors",
        purpose="CONTINUOUS",
        result_fields=["Test Case", "Test Config", "Status"],
    ),
    TestTypeConfig(
        test_type="Data Corral Tests",
        scuba_table="netcastle_test_results",
        team="fboss_data_corral",
        purpose="CONTINUOUS",
        result_fields=["Test Case", "Test Config", "Status"],
    ),
    TestTypeConfig(
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
    ),
    TestTypeConfig(
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
    ),
    TestTypeConfig(
        test_type="Firmware Util",
        scuba_table="netcastle_test_results",
        team="fboss_fw_util",
        purpose="CONTINUOUS",
        result_fields=["Test Case", "Test Config", "Status"],
    ),
]


def should_skip_test_case(test_case: str) -> bool:
    """Return True if the test case name matches any skip pattern."""
    for pattern in SKIP_PATTERNS:
        if pattern.lower() in test_case.lower():
            return True
    return False


def has_null_fields(row: Any, fields: List[str]) -> bool:
    """Return True if any required field in the row is null/NaN."""
    for f in fields:
        val = row.get(f)
        if val is None:
            return True
        if isinstance(val, float) and pd.isna(val):
            return True
    return False


def build_scuba_query(config: TestTypeConfig) -> str:
    """Build the Scuba SQL query for a given test type.

    Always includes `time` for deduplication (keeping the latest run per test case).
    """
    current_time = int(time.time())
    start_time = current_time - QUERY_LOOKBACK_SECONDS
    fields_select = ", ".join(f"`{f}`" for f in config.result_fields)
    return f"""
        SELECT {fields_select}, `time`
        FROM `{config.scuba_table}`
        WHERE
            {start_time} <= `time`
            AND `time` <= {current_time}
            AND `Team` = '{config.team}'
            AND `Purpose` = '{config.purpose}'
    """


def query_test_type(config: TestTypeConfig) -> Optional[pd.DataFrame]:
    """Query Scuba for a single test type. Returns None if no results."""
    query = build_scuba_query(config)
    logger.info(f"Querying {config.test_type} from {config.scuba_table}...")
    try:
        df = bb.query_scuba_nullable(query)
    except Exception as e:
        logger.error(f"Error querying {config.test_type}: {e}")
        return None
    if df is None or df.empty:
        logger.info(f"No results for {config.test_type}, skipping.")
        return None
    return df


def is_null_string(val: Any) -> bool:
    """Return True if a value is the literal string 'null' (case-insensitive)."""
    return isinstance(val, str) and val.strip().lower() == "null"


def filter_results(df: pd.DataFrame, config: TestTypeConfig) -> pd.DataFrame:
    """Apply skip rules: remove null fields, null test configs, and matching test case names."""
    rows_to_keep = []
    for idx, row in df.iterrows():
        if has_null_fields(row, config.result_fields):
            continue
        test_config = row.get("Test Config", "")
        if is_null_string(test_config):
            continue
        test_case = str(row.get("Test Case", ""))
        if should_skip_test_case(test_case):
            continue
        rows_to_keep.append(idx)
    return df.loc[rows_to_keep]


def deduplicate_results(df: pd.DataFrame) -> pd.DataFrame:
    """Keep only the latest result per (Test Case, Test Config) pair.

    Multiple CI runs within the lookback window can produce duplicate rows
    for the same test case. We sort by time descending and keep the first
    occurrence of each (Test Case, Test Config) combination.
    """
    df = df.sort_values("time", ascending=False)
    df = df.drop_duplicates(subset=["Test Case", "Test Config"], keep="first")
    df = df.drop(columns=["time"])
    return df


def sanitize_filename(name: str) -> str:
    """Convert a name to a safe filename."""
    return re.sub(r"[^a-zA-Z0-9_-]", "_", name.lower().strip())


def _parse_agent_or_sai(parts: List[str]) -> Dict[str, str]:
    """Parse config for Agent HW Test / SAI Test: vendor/from_sdk/to_sdk/asic/mode[/role]."""
    result: Dict[str, str] = {}
    if len(parts) > 0:
        result["Vendor"] = VENDOR_NAMES.get(parts[0], parts[0])
    if len(parts) > 1:
        result["SDK Version"] = parts[1]
    if len(parts) > 3:
        result["ASIC"] = parts[3]
    if len(parts) > 4:
        result["Switch Mode"] = parts[4].replace("_", " ")
    result["Role"] = parts[5].replace("_", " ") if len(parts) > 5 else ""
    return result


def _parse_agent_benchmarks(parts: List[str]) -> Dict[str, str]:
    """Parse config for Agent Benchmarks: vendor/sdk/asic/mode[/role]."""
    result: Dict[str, str] = {}
    if len(parts) > 0:
        result["Vendor"] = VENDOR_NAMES.get(parts[0], parts[0])
    if len(parts) > 1:
        result["SDK Version"] = parts[1]
    if len(parts) > 2:
        result["ASIC"] = parts[2]
    if len(parts) > 3:
        result["Switch Mode"] = parts[3].replace("_", " ")
    result["Role"] = parts[4].replace("_", " ") if len(parts) > 4 else ""
    return result


def _parse_link_tests(parts: List[str]) -> Dict[str, str]:
    """Parse config for Link Tests: platform/sai/asicsdk-version/version."""
    result: Dict[str, str] = {"Platform": parts[0]}
    if len(parts) > 2:
        result["ASIC SDK"] = parts[2].replace("asicsdk-", "")
    return result


def _parse_qsfp_hw_tests(parts: List[str]) -> Dict[str, str]:
    """Parse config for Qsfp HW Tests: platform/physdk-vendor-version/vendor-version."""
    result: Dict[str, str] = {"Platform": parts[0]}
    if len(parts) > 1:
        result["PHY SDK"] = parts[1].replace("physdk-", "")
    return result


def _parse_fsdb_benchmarks(parts: List[str]) -> Dict[str, str]:
    """Parse config for FSDB Benchmarks: component/platform/sdk/vendor."""
    result: Dict[str, str] = {}
    if len(parts) > 0:
        result["Component"] = parts[0]
    if len(parts) > 1:
        result["Platform"] = parts[1]
    if len(parts) > 2:
        result["SDK Version"] = parts[2]
    if len(parts) > 3:
        result["Vendor"] = VENDOR_NAMES.get(parts[3], parts[3])
    return result


def _parse_qsfp_benchmarks(parts: List[str]) -> Dict[str, str]:
    """Parse config for Qsfp Benchmarks: platform/phy_version."""
    result: Dict[str, str] = {"Platform": parts[0]}
    if len(parts) > 1:
        result["PHY SDK"] = parts[1]
    return result


_SIMPLE_PLATFORM_TYPES = frozenset(
    {
        "BSP Tests",
        "LED Tests",
        "Platform Manager",
        "Fan Tests",
        "Sensors Tests",
        "Data Corral Tests",
        "Firmware Util",
    }
)

_CONFIG_PARSERS: Dict[str, Callable[[List[str]], Dict[str, str]]] = {
    "Agent HW Test": _parse_agent_or_sai,
    "SAI Test": _parse_agent_or_sai,
    "Agent Benchmarks": _parse_agent_benchmarks,
    "Link Tests": _parse_link_tests,
    "Qsfp HW Tests": _parse_qsfp_hw_tests,
    "FSDB Benchmarks": _parse_fsdb_benchmarks,
    "Qsfp Benchmarks": _parse_qsfp_benchmarks,
}


def parse_test_config(test_type: str, config: str) -> Dict[str, str]:
    """Parse a test config string into semantic columns based on test type.

    Different test types encode different information in their config strings:
      - Agent HW Test / SAI Test: vendor/from_sdk/to_sdk/asic/mode[/role]
      - Agent Benchmarks: vendor/sdk/asic/mode[/role]
      - Link Tests: platform/sai/asicsdk-version/version
      - Qsfp HW Tests: platform/physdk-vendor-version/vendor-version
      - FSDB Benchmarks: component/platform/sdk/vendor
      - Qsfp Benchmarks: platform/phy_version
      - BSP/LED/Fan/Sensors/DataCorral/PlatformManager/FirmwareUtil: platform
    """
    parts = config.split("/")

    if test_type in _SIMPLE_PLATFORM_TYPES:
        return {"Platform": parts[0]}

    parser = _CONFIG_PARSERS.get(test_type)
    if parser is not None:
        return parser(parts)

    # Fallback
    return {"Platform": config}


def should_skip_config(parsed: Dict[str, str]) -> bool:
    """Return True if the parsed config matches any platform or ASIC skip rule."""
    platform = parsed.get("Platform", "")
    if platform and platform.lower() in {p.lower() for p in SKIP_PLATFORMS}:
        return True
    asic = parsed.get("ASIC", "")
    if asic and asic.lower() in {a.lower() for a in SKIP_ASICS}:
        return True
    return False


def get_csv_output_fields(config: TestTypeConfig) -> List[str]:
    """Return the CSV output columns, replacing 'Test Config' with parsed columns.

    Also removes Scuba fields that are redundant with parsed columns
    (e.g. Scuba 'Asic' is dropped when parsed columns include 'ASIC').
    """
    parsed_cols = CONFIG_COLUMNS.get(config.test_type, ["Platform"])
    parsed_lower = {c.lower() for c in parsed_cols}
    output: List[str] = []
    for f in config.result_fields:
        if f == "Test Config":
            output.extend(parsed_cols)
        elif f.lower() in parsed_lower:
            continue
        else:
            output.append(f)
    return output


def write_csvs_for_test_type(
    df: pd.DataFrame,
    config: TestTypeConfig,
    output_dir: str,
) -> List[Dict[str, Any]]:
    """Group results by Test Config, write one CSV per config.
    Replaces the raw 'Test Config' column with parsed semantic columns
    (e.g. Vendor, SDK Version, ASIC, Platform) based on the test type.
    Returns list of dicts with metadata for the summary."""
    csv_metadata = []
    output_fields = get_csv_output_fields(config)
    for test_config_val, group_df in df.groupby("Test Config"):
        # Parse the config string into semantic columns
        parsed = parse_test_config(config.test_type, str(test_config_val))

        # Skip configs whose platform or ASIC is in the skip lists
        if should_skip_config(parsed):
            logger.info(
                f"  Skipping {config.test_type}/{test_config_val} (matches skip list)"
            )
            continue

        group_df = group_df.copy()
        for col_name, col_value in parsed.items():
            group_df[col_name] = col_value

        safe_type = sanitize_filename(config.test_type)
        safe_config = sanitize_filename(str(test_config_val))
        filename = f"{safe_type}_{safe_config}.csv"
        filepath = os.path.join(output_dir, filename)
        cols = [f for f in output_fields if f in group_df.columns]
        group_df[cols].to_csv(filepath, index=False, quoting=csv.QUOTE_ALL)
        logger.info(f"  Wrote {filepath} ({len(group_df)} rows)")
        entry = {
            "test_type": config.test_type,
            "test_config": str(test_config_val),
            "csv_file": filename,
            "total": len(group_df),
        }
        if "Status" in group_df.columns:
            passed = int((group_df["Status"] == "PASSED").sum())
            failed = len(group_df) - passed
            entry["passed"] = passed
            entry["failed"] = failed
            entry["pass_pct"] = (
                round(passed / len(group_df) * 100, 1) if len(group_df) > 0 else 0
            )
        csv_metadata.append(entry)
    return csv_metadata


def generate_markdown(
    type_summary: List[Dict[str, Any]],
    all_details: List[Dict[str, Any]],
    docs_output_dir: str,
) -> None:
    """Generate the test_results.mdx page for the Docusaurus site.

    Produces an MDX file that imports the TestResultsDashboard React component
    and passes the summary and detail data as JSON props.  This gives us an
    interactive, color-coded dashboard instead of a plain markdown table.
    """
    import json as _json

    # Build summary data for the component
    summary_data = []
    for entry in type_summary:
        summary_data.append(
            {
                "name": entry["test_type"],
                "total": entry["total"],
                "passed": entry.get("passed"),
                "failed": entry.get("failed"),
                "pct": entry.get("pass_pct"),
            }
        )

    # Build details data: [test_type, test_config, csv_filename]
    details_data = []
    for entry in all_details:
        details_data.append(
            [entry["test_type"], entry["test_config"], entry["csv_file"]]
        )

    summary_json = _json.dumps(summary_data)
    details_json = _json.dumps(details_data)

    lines = [
        "---",
        "id: test_results",
        "title: FBOSS Test Results",
        "description: FBOSS continuous test results with CSV downloads",
        "keywords:",
        "    - FBOSS",
        "    - OSS",
        "    - test",
        "    - results",
        "oncall: fboss_oss",
        "---",
        "",
        "import TestResultsDashboard from '@site/src/components/TestResultsDashboard';",
        "",
        f"<TestResultsDashboard summary={{{summary_json}}} details={{{details_json}}} />",
        "",
    ]
    filepath = os.path.join(docs_output_dir, "test_results.mdx")
    os.makedirs(docs_output_dir, exist_ok=True)
    with open(filepath, "w") as f:
        f.write("\n".join(lines))
    logger.info(f"MDX page written to {filepath}")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Export FBOSS test results from Scuba to CSV for OSS wiki."
    )
    parser.add_argument(
        "--csv_output_dir",
        default=DEFAULT_CSV_OUTPUT_DIR,
        help="Directory to write CSV files (Docusaurus static assets).",
    )
    parser.add_argument(
        "--docs_output_dir",
        default=DEFAULT_DOCS_OUTPUT_DIR,
        help="Directory to write the generated markdown page.",
    )
    args = parser.parse_args()

    csv_output_dir = os.path.abspath(args.csv_output_dir)
    docs_output_dir = os.path.abspath(args.docs_output_dir)
    os.makedirs(csv_output_dir, exist_ok=True)

    # Clean up previous CSV files so stale configs don't linger
    for f in os.listdir(csv_output_dir):
        if f.endswith(".csv"):
            os.remove(os.path.join(csv_output_dir, f))
    logger.info(f"Cleared existing CSV files from {csv_output_dir}")

    all_details: List[Dict[str, Any]] = []
    type_summary: List[Dict[str, Any]] = []

    for config in TEST_TYPE_CONFIGS:
        df = query_test_type(config)
        if df is None:
            continue
        df = filter_results(df, config)
        if df.empty:
            logger.info(f"No results after filtering for {config.test_type}, skipping.")
            continue
        df = deduplicate_results(df)
        csv_meta = write_csvs_for_test_type(df, config, csv_output_dir)
        all_details.extend(csv_meta)
        total = sum(e["total"] for e in csv_meta)
        has_status = any("passed" in e for e in csv_meta)
        type_entry: Dict[str, Any] = {
            "test_type": config.test_type,
            "total": total,
        }
        if has_status:
            passed = sum(e.get("passed", 0) for e in csv_meta)
            failed = sum(e.get("failed", 0) for e in csv_meta)
            type_entry["passed"] = passed
            type_entry["failed"] = failed
            type_entry["pass_pct"] = round(passed / total * 100, 1) if total > 0 else 0
        type_summary.append(type_entry)

    generate_markdown(type_summary, all_details, docs_output_dir)
    logger.info(f"Done. {len(all_details)} CSV files generated.")


if __name__ == "__main__":
    main()
