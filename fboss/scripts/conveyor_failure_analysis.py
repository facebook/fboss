#!/usr/bin/env python3
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

"""
Analyze failures in FBOSS conveyor pipelines and produce a categorized report.

Usage:
    buck run fbcode//fboss/scripts:conveyor_failure_analysis -- \
        --conveyor-id fboss/forwarding_stack_integ_test \
        --limit 10 \
        --output /tmp/failure_report.md

    # Filter to a specific pipeline (default: blocking nodes only)
    buck run fbcode//fboss/scripts:conveyor_failure_analysis -- \
        --conveyor-id fboss/forwarding_stack_integ_test \
        --pipeline "Integration Tests" \
        --limit 10

    # Sample N failures per category for deep-dive log analysis
    buck run fbcode//fboss/scripts:conveyor_failure_analysis -- \
        --conveyor-id fboss/forwarding_stack_integ_test \
        --limit 10 \
        --sample 2
"""

import argparse
import json
import re
import subprocess
import sys
from collections import Counter, defaultdict
from datetime import datetime, timezone


def run_conveyor_cmd(args: list[str]) -> str:
    """Run a conveyor CLI command and return stdout."""
    result = subprocess.run(args, capture_output=True, text=True, timeout=120)
    if result.returncode != 0:
        print(f"Warning: conveyor command failed: {' '.join(args)}", file=sys.stderr)
        print(result.stderr[:500], file=sys.stderr)
    return result.stdout


def fetch_releases(conveyor_id: str, limit: int) -> list[dict]:
    """Fetch release data from conveyor CLI."""
    cmd = [
        "conveyor",
        "release",
        "status",
        "--conveyor-id",
        conveyor_id,
        "--limit",
        str(limit),
        "--json",
    ]
    output = run_conveyor_cmd(cmd)
    return json.loads(output)


def fetch_owner_nodes(conveyor_id: str, owner: str) -> set[str] | None:
    """Fetch the set of nodes belonging to an owner group from conveyor config.

    The conveyor config defines groups like "Owner fboss_optics_phy Tests" with
    ordered_node_names. Returns the set of node names, or None if not found.
    """
    cmd = ["conveyor", "info", "--conveyor-id", conveyor_id, "--json"]
    output = run_conveyor_cmd(cmd)
    if not output:
        return None
    data = json.loads(output)
    config = (
        data.get("Conveyor Config", {}).get("config", {}).get("conveyor3_config", {})
    )
    groups = config.get("groups", [])
    target = f"Owner {owner} Tests".lower()
    for g in groups:
        cng = g.get("collapsible_node_group", {}).get("group", {})
        name = cng.get("pipeline_group_name", "")
        if name.lower() == target:
            nodes = cng.get("pipeline_node_group", {}).get("ordered_node_names", [])
            # Filter out meta-nodes like "Owner:fboss_optics_phy"
            return {n for n in nodes if not n.startswith("Owner:")}

    # Fallback: try substring match
    for g in groups:
        cng = g.get("collapsible_node_group", {}).get("group", {})
        name = cng.get("pipeline_group_name", "")
        if owner.lower() in name.lower():
            nodes = cng.get("pipeline_node_group", {}).get("ordered_node_names", [])
            return {n for n in nodes if not n.startswith("Owner:")}

    return None


def determine_blocking_nodes(
    data: list[dict],
) -> set[str]:
    """
    Determine which nodes belong to the blocking pipeline by finding the
    intersection of nodes across releases with a "normal" node count.

    The conveyor UI filters by pipeline. Releases that have a drastically
    different node set (e.g., 185 vs 140) belong to a different pipeline
    instance and should be excluded. We detect the blocking pipeline by
    finding the common set of nodes across releases with similar node counts.
    """
    nodes_by_release: dict[int, set[str]] = defaultdict(set)
    for item in data:
        runs = item.get("runs") or {}
        for node, run in runs.items():
            if not run:
                continue
            release = run.get("release_number")
            if release is not None:
                nodes_by_release[release].add(node)

    if not nodes_by_release:
        return set()

    # Find the median node count to determine "normal" releases
    counts = sorted(len(v) for v in nodes_by_release.values())
    median_count = counts[len(counts) // 2]
    threshold = median_count * 1.15  # allow 15% variance

    normal_releases = [
        r for r, nodes in nodes_by_release.items() if len(nodes) <= threshold
    ]

    if not normal_releases:
        # Fall back to all releases
        normal_releases = list(nodes_by_release.keys())

    # Intersection of nodes across normal releases = blocking pipeline
    blocking = nodes_by_release[normal_releases[0]].copy()
    for r in normal_releases[1:]:
        blocking &= nodes_by_release[r]

    return blocking


def extract_failures(
    data: list[dict],
    blocking_nodes: set[str] | None,
) -> list[dict]:
    """Extract all non-successful runs, optionally filtered to blocking nodes."""
    skip_statuses = {
        "SUCCESSFUL",
        "RUNNING",
        "NOT_STARTED",
        "SKIPPED",
        "ONGOING",
        "SCHEDULED",
    }
    failures = []

    for item in data:
        runs = item.get("runs") or {}
        for node, run in runs.items():
            if not run:
                continue
            status = run.get("status", "")
            if status in skip_statuses:
                continue
            if blocking_nodes is not None and node not in blocking_nodes:
                continue

            release = run.get("release_number")

            # Compute elapsed time
            elapsed_s = 0
            start_time = run.get("actual_start_time", {})
            end_time = run.get("actual_end_time") or run.get("last_update_time") or {}
            if start_time and end_time:
                s = start_time.get("secs", 0)
                e = end_time.get("secs", 0)
                if s and e:
                    elapsed_s = e - s

            # Extract SC job ID from run_data
            sc_job = ""
            run_data_str = run.get("run_data", "")
            if run_data_str:
                try:
                    rd = json.loads(run_data_str)
                    sc_job = str(rd.get("sandcastle_job_id", ""))
                except (json.JSONDecodeError, TypeError):
                    pass

            # Extract test result summary from run_details
            details = run.get("run_details", "")
            failed_summary = ""
            passed_summary = ""
            failed_tests: list[str] = []
            everpaste_urls: list[str] = []

            if details:
                fm = re.findall(r"\*\*FAILED:\s*(\d+)\s*\((\d+)%\)\*\*", details)
                if fm:
                    failed_summary = f"FAILED: {fm[0][0]} ({fm[0][1]}%)"
                pm = re.findall(r"\*\*PASSED:\s*(\d+)\s*\((\d+)%\)\*\*", details)
                if pm:
                    passed_summary = f"PASSED: {pm[0][0]} ({pm[0][1]}%)"

                # Extract failed test names and their everpaste URLs
                # Look for the FAILED section
                failed_section = re.search(
                    r"\*\*FAILED:.*?\*\*(.*?)(?:\*\*PASSED|\*\*SKIPPED|\Z)",
                    details,
                    re.DOTALL,
                )
                if failed_section:
                    section = failed_section.group(1)
                    # Extract test names
                    test_names = re.findall(r"\*\*(.+?)\*\*\s*\(", section)
                    failed_tests = test_names
                    # Extract everpaste detail URLs
                    everpaste_urls = re.findall(
                        r"https://fburl\.com/everpaste/\w+", section
                    )

            # Extract fbpkg versions from run_data cmd
            fbpkg_versions: dict[str, str] = {}
            if run_data_str:
                try:
                    rd = json.loads(run_data_str)
                    cmd = rd.get("cmd", "")
                    versions = re.findall(r"(neteng\.fboss\.\w+):(\w{8,})", cmd)
                    fbpkg_versions = dict(versions)
                except (json.JSONDecodeError, TypeError):
                    pass

            failures.append(
                {
                    "release": release,
                    "node": node,
                    "status": status,
                    "elapsed_s": elapsed_s,
                    "elapsed_str": format_elapsed(elapsed_s),
                    "sc_job": sc_job,
                    "failed_summary": failed_summary,
                    "passed_summary": passed_summary,
                    "failed_tests": failed_tests,
                    "everpaste_urls": everpaste_urls,
                    "fbpkg_versions": fbpkg_versions,
                }
            )

    failures.sort(key=lambda x: (x["release"] or 0, x["node"]))
    return failures


def format_elapsed(seconds: int) -> str:
    """Format elapsed seconds as human-readable string."""
    if seconds == 0:
        return "0s"
    m, s = divmod(seconds, 60)
    h, m = divmod(m, 60)
    if h:
        return f"{h}h{m:02d}m"
    return f"{m}m{s:02d}s"


def classify_failures(failures: list[dict]) -> list[dict]:
    """
    Classify failures into categories based on patterns.
    Returns a list of category dicts.
    """
    categories: dict[str, dict] = {}

    for f in failures:
        key = categorize_single_failure(f)
        if key not in categories:
            categories[key] = {
                "name": key,
                "type": "INFRA"
                if f["status"] == "INFRA_ERROR"
                else "TEST FAILURE"
                if f["status"] == "NODE_ERROR"
                else "APPLICATION FAILURE",
                "failures": [],
            }
        categories[key]["failures"].append(f)

    return sorted(categories.values(), key=lambda c: -len(c["failures"]))


def _parse_node_name(node: str) -> dict[str, str]:
    """Parse a conveyor node name into structured components."""
    result: dict[str, str] = {"raw": node}

    m = re.match(
        r"(\w+?)_ensemble_link_(mono|multi_switch)_test_"
        r"(?:preprod2trunk2preprod_)?"
        r"(.+?)_L(\d+)(?:_(.+))?$",
        node,
    )
    if m:
        result.update(
            {
                "type": "ensemble_link",
                "platform": m.group(1),
                "mode": m.group(2),
                "sdk": m.group(3),
                "tier": f"L{m.group(4)}",
            }
        )
        return result

    m = re.match(r"(?:netos_)?fboss_qsfp_pm_test_(\w+?)_(.+)$", node)
    if m:
        result.update({"type": "qsfp_pm", "platform": m.group(1), "sdk": m.group(2)})
        return result

    if "fsdb_integration_test" in node:
        result["type"] = "fsdb_integration"
        return result
    if "validate_agent_config" in node:
        result["type"] = "validate_config"
        return result

    result["type"] = "other"
    return result


def categorize_single_failure(f: dict) -> str:
    """Assign a category name to a single failure based on its properties."""
    node = f["node"]
    status = f["status"]
    elapsed = f["elapsed_s"]
    parsed = _parse_node_name(node)

    if status == "APPLICATION_FAILURE":
        if parsed["type"] == "validate_config":
            return "Agent config validation failure"
        return f"Application failure: {node}"

    if status == "INFRA_ERROR":
        is_deadline = elapsed >= 21000

        if parsed["type"] == "ensemble_link":
            platform = parsed["platform"]
            mode = parsed["mode"]
            sdk = parsed["sdk"]
            if is_deadline:
                return f"{platform} {mode} test deadline exceeded ({sdk})"
            return f"{platform} {mode} infra error ({sdk})"

        if parsed["type"] == "qsfp_pm":
            return f"QSFP PM test infra error ({parsed['platform']})"

        if is_deadline:
            return f"Deadline exceeded: {node}"
        return f"Infra error: {node}"

    if status == "NODE_ERROR":
        if parsed["type"] == "ensemble_link":
            platform = parsed["platform"]
            mode = parsed["mode"]
            sdk = parsed["sdk"]
            return f"{platform} {mode} test failure ({sdk})"

        if parsed["type"] == "qsfp_pm":
            return f"QSFP PM test failure ({parsed['platform']})"

        if parsed["type"] == "fsdb_integration":
            return "FSDB integration test failure"

        return f"Test failure: {node}"

    return f"Unknown: {node}"


def fetch_everpaste(url: str) -> str | None:
    """Fetch everpaste content using curl with cookie auth. Returns text or None."""
    try:
        import os

        username = os.environ.get("USER", os.environ.get("LOGNAME", "nobody"))
        result = subprocess.run(
            [
                "curl",
                "-sL",
                "--cookie",
                f"/var/tmp/cookies-{username}",
                "--max-time",
                "30",
                url,
            ],
            capture_output=True,
            text=True,
            timeout=35,
        )
        if (
            result.returncode == 0
            and result.stdout
            and "<html" not in result.stdout[:200].lower()
        ):
            return result.stdout
        return None
    except Exception:
        return None


def extract_failure_signature_from_everpaste(content: str) -> str:
    r"""Extract the key failure assertion from everpaste content."""
    lines = content.split("\n")
    signatures = []

    for i, line in enumerate(lines):
        # Look for gtest assertion failures
        if re.search(r"\.cpp:\d+: Failure", line):
            # Collect the failure + next few lines (Value of, Actual, Expected)
            context = [line.strip()]
            for j in range(i + 1, min(i + 5, len(lines))):
                stripped = lines[j].strip()
                if stripped.startswith(("Value of:", "Actual:", "Expected:")):
                    context.append(stripped)
                elif stripped and not stripped.startswith(("V0", "I0", "W0", "E0")):
                    context.append(stripped)
                else:
                    break
            signatures.append("\n".join(context))

    if signatures:
        # Return unique signatures
        seen = set()
        unique = []
        for s in signatures:
            if s not in seen:
                seen.add(s)
                unique.append(s)
        return "\n\n".join(unique[:3])  # Max 3 unique signatures

    # Fallback: look for FATAL, SIGABRT, etc.
    for line in lines:
        if re.search(r"FATAL|SIGABRT|SIGSEGV|switch initialization failed", line):
            return line.strip()

    return ""


def deep_dive_category(category: dict, sample_size: int) -> dict:
    """
    Deep-dive into a category by fetching everpaste logs for sample failures.
    Returns the category dict with added 'log_signature' field.
    """
    failures_with_urls = [f for f in category["failures"] if f["everpaste_urls"]]

    if not failures_with_urls:
        category["log_signature"] = ""
        return category

    # Sample up to sample_size failures
    sampled = failures_with_urls[:sample_size]
    all_signatures = []

    for f in sampled:
        for url in f["everpaste_urls"][:2]:  # Max 2 URLs per failure
            content = fetch_everpaste(url)
            if content:
                sig = extract_failure_signature_from_everpaste(content)
                if sig:
                    all_signatures.append(sig)

    # Deduplicate and combine
    seen = set()
    unique_sigs = []
    for s in all_signatures:
        if s not in seen:
            seen.add(s)
            unique_sigs.append(s)

    category["log_signature"] = "\n\n".join(unique_sigs[:3])
    return category


def investigate_category(category: dict, all_failures: list[dict]) -> dict:
    """Deep investigation: extract devices, assertions, fbpkg version changes."""
    cat_failures = category["failures"]
    devices: list[str] = []
    assertion_details: list[str] = []

    for f in cat_failures[:3]:
        for url in f.get("everpaste_urls", [])[:1]:
            content = fetch_everpaste(url)
            if not content:
                continue
            device_match = re.search(r"Switch:\s*(\S+)", content)
            if device_match:
                devices.append(device_match.group(1))
            stdout_url = re.search(r"Full:\s*(https://\S+everpaste\S+)", content)
            if stdout_url:
                stdout = fetch_everpaste(stdout_url.group(1))
                if stdout:
                    for i, line in enumerate(stdout.split("\n")):
                        if re.search(r"\.cpp:\d+: Failure", line):
                            ctx = [line.strip()]
                            lines = stdout.split("\n")
                            for j in range(i + 1, min(i + 5, len(lines))):
                                s = lines[j].strip()
                                if s.startswith(("Value of:", "Actual:", "Expected:")):
                                    ctx.append(s)
                                elif s and not s.startswith(("V0", "I0", "W0", "E0")):
                                    ctx.append(s)
                                else:
                                    break
                            assertion_details.append("\n".join(ctx))
                        elif "TTransportException" in line and "Failure" not in line:
                            assertion_details.append(line.strip())

    unique_devices = list(dict.fromkeys(devices))
    unique_assertions = list(dict.fromkeys(assertion_details))

    # Track fbpkg version changes
    node_name = cat_failures[0]["node"] if cat_failures else ""
    version_history: list[dict] = []
    seen_releases: set[int] = set()
    for f in all_failures:
        if f["node"] == node_name and f["release"] not in seen_releases:
            seen_releases.add(f["release"])
            version_history.append(
                {
                    "release": f["release"],
                    "status": f["status"],
                    "versions": f.get("fbpkg_versions", {}),
                }
            )
    version_history.sort(key=lambda x: x["release"] or 0)

    category["investigation"] = {
        "devices": unique_devices,
        "assertions": unique_assertions[:3],
        "version_history": version_history,
    }
    return category


def generate_investigation_section(category: dict) -> list[str]:
    """Generate markdown for an investigation section."""
    inv = category.get("investigation")
    if not inv:
        return []

    lines = []
    lines.append("**Investigation**:")
    lines.append("")

    devices = inv.get("devices", [])
    if devices:
        unique = list(dict.fromkeys(devices))
        if len(unique) == 1:
            lines.append(
                f"- **Device**: Always `{unique[0]}` — likely hardware-specific"
            )
        else:
            lines.append(
                f"- **Devices**: {', '.join(f'`{d}`' for d in unique)} — multiple devices affected (software issue)"
            )
    lines.append("")

    assertions = inv.get("assertions", [])
    if assertions:
        lines.append("- **Assertion failures**:")
        lines.append("```")
        for a in assertions:
            lines.append(a)
            lines.append("")
        lines.append("```")

    history = inv.get("version_history", [])
    if len(history) >= 2:
        first = history[0].get("versions", {})
        last = history[-1].get("versions", {})
        changed = []
        for pkg in set(first) | set(last):
            v1 = first.get(pkg, "?")
            v2 = last.get(pkg, "?")
            if v1 != v2:
                changed.append(f"  - `{pkg}`: `{v1[:12]}` → `{v2[:12]}`")
        if changed:
            lines.append(
                f"- **Package changes** (R{history[0]['release']} → R{history[-1]['release']}):"
            )
            lines.extend(changed)
            lines.append("")

    return lines


def generate_report(
    conveyor_id: str,
    categories: list[dict],
    failures: list[dict],
    releases_analyzed: list[int],
    owner: str | None = None,
    num_owner_nodes: int | None = None,
) -> str:
    """Generate the markdown report."""
    total = len(failures)
    unique_nodes = len({f["node"] for f in failures})
    infra_count = sum(1 for f in failures if f["status"] == "INFRA_ERROR")
    node_error_count = sum(1 for f in failures if f["status"] == "NODE_ERROR")
    app_failure_count = sum(1 for f in failures if f["status"] == "APPLICATION_FAILURE")
    release_counts = Counter(f["release"] for f in failures)
    worst_release = (
        # pyrefly: ignore [no-matching-overload]
        max(release_counts, key=release_counts.get) if release_counts else None
    )

    r_start = min(releases_analyzed) if releases_analyzed else "?"
    r_end = max(releases_analyzed) if releases_analyzed else "?"
    date = datetime.now(timezone.utc).strftime("%Y-%m-%d")

    title_suffix = f" — {owner}" if owner else ""
    lines = []
    lines.append(f"# {conveyor_id}{title_suffix} Failure Analysis")
    lines.append("")
    lines.append(f"**Conveyor**: `{conveyor_id}`")
    pipeline_desc = "Integration Tests (blocking nodes only)"
    if owner:
        node_count = f" ({num_owner_nodes} nodes)" if num_owner_nodes else ""
        pipeline_desc += f", Owner: `{owner}`{node_count}"
    lines.append(f"**Pipeline**: {pipeline_desc}")
    lines.append(f"**Releases Analyzed**: R{r_start} – R{r_end}")
    lines.append(f"**Date**: {date}")
    lines.append("")
    lines.append("---")
    lines.append("")
    lines.append("## Executive Summary")
    lines.append("")

    type_breakdown = []
    if infra_count:
        type_breakdown.append(
            f"{infra_count} INFRA_ERRORs ({infra_count * 100 // total}%)"
        )
    if node_error_count:
        type_breakdown.append(
            f"{node_error_count} NODE_ERRORs ({node_error_count * 100 // total}%)"
        )
    if app_failure_count:
        type_breakdown.append(f"{app_failure_count} APPLICATION_FAILUREs")

    lines.append(
        f"**{total} failures** across {len(release_counts)} releases "
        f"({unique_nodes} unique nodes). {', '.join(type_breakdown)}."
    )
    lines.append("")

    # Note outlier releases
    avg = total / max(len(release_counts), 1)
    outliers = [f"R{r}" for r, c in release_counts.items() if c > avg * 2]
    if outliers:
        lines.append(
            f"Outlier releases with disproportionate failures: {', '.join(outliers)}."
        )
        lines.append("")

    lines.append("---")
    lines.append("")
    lines.append("## Failure Categories")
    lines.append("")

    for i, cat in enumerate(categories):
        letter = chr(ord("A") + i)
        cat_failures = cat["failures"]
        lines.append(f"### Category {letter}: {cat['name']}")
        lines.append(f"**Type**: {cat['type']}")
        lines.append(f"**Count**: {len(cat_failures)} occurrences")
        lines.append("")

        # Log signature
        sig = cat.get("log_signature", "")
        if sig:
            lines.append("**Log Signature**:")
            lines.append("```")
            lines.append(sig)
            lines.append("```")
            lines.append("")

        # Failure table
        lines.append("| Release | Node | Elapsed | Result | SC Job |")
        lines.append("|---------|------|---------|--------|--------|")
        for f in cat_failures:
            sc_link = (
                f"[{f['sc_job']}](https://www.internalfb.com/intern/sandcastle/job/{f['sc_job']})"
                if f["sc_job"]
                else "—"
            )
            result = f["failed_summary"] or f["status"]
            node_short = f["node"][:70]
            lines.append(
                f"| R{f['release']} | {node_short} | {f['elapsed_str']} | {result} | {sc_link} |"
            )

        # List failed tests if available
        all_failed_tests: list[str] = []
        for f in cat_failures:
            all_failed_tests.extend(f["failed_tests"])
        if all_failed_tests:
            test_counts = Counter(all_failed_tests)
            lines.append("")
            lines.append("**Failing tests** (by frequency):")
            for test, count in test_counts.most_common(10):
                # Shorten the test name
                short = test.split(" - ")[-1] if " - " in test else test
                lines.append(f"- `{short}` ({count}x)")

        # Investigation details (if --investigate was used)
        inv_lines = generate_investigation_section(cat)
        if inv_lines:
            lines.append("")
            lines.extend(inv_lines)

        lines.append("")
        lines.append("---")
        lines.append("")

    # Summary statistics
    lines.append("## Summary Statistics")
    lines.append("")
    lines.append("| Metric | Value |")
    lines.append("|--------|-------|")
    lines.append(f"| Total failures | {total} |")
    lines.append(f"| Unique failing nodes | {unique_nodes} |")
    lines.append(f"| Infra failures (INFRA_ERROR) | {infra_count} |")
    lines.append(f"| Test failures (NODE_ERROR) | {node_error_count} |")
    if app_failure_count:
        lines.append(f"| Application failures | {app_failure_count} |")
    lines.append(
        f"| Releases with failures | {len(release_counts)} of {len(releases_analyzed)} |"
    )
    if worst_release is not None:
        lines.append(
            f"| Worst release | R{worst_release} ({release_counts[worst_release]} failures) |"
        )
    lines.append("")
    lines.append("---")
    lines.append("")

    # Recommendations
    lines.append("## Recommendations")
    lines.append("")
    for i, cat in enumerate(categories):
        letter = chr(ord("A") + i)
        name = cat["name"]
        count = len(cat["failures"])
        cat_type = cat["type"]

        if "deadline exceeded" in name.lower():
            lines.append(
                f"{i + 1}. **Category {letter} ({name})**: "
                f"Consistently hitting deadline ({count}x). "
                f"Consider splitting the test suite or increasing the deadline."
            )
        elif cat_type == "TEST FAILURE":
            top_tests = Counter()
            for f in cat["failures"]:
                top_tests.update(f["failed_tests"])
            if top_tests:
                top_test = top_tests.most_common(1)[0][0]
                short = top_test.split(" - ")[-1] if " - " in top_test else top_test
                lines.append(
                    f"{i + 1}. **Category {letter} ({name})**: "
                    f"Top failing test: `{short}`. "
                    f"File a targeted bug or consider quarantining if flaky."
                )
            else:
                lines.append(
                    f"{i + 1}. **Category {letter} ({name})**: "
                    f"Investigate test failures ({count} occurrences)."
                )
        else:
            lines.append(
                f"{i + 1}. **Category {letter} ({name})**: "
                f"Investigate root cause ({count} occurrences)."
            )

    lines.append("")
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Analyze FBOSS conveyor pipeline failures"
    )
    parser.add_argument(
        "--conveyor-id",
        required=True,
        help="Conveyor ID (e.g., fboss/forwarding_stack_integ_test)",
    )
    parser.add_argument(
        "--limit",
        type=int,
        default=10,
        help="Number of recent releases to analyze (default: 10)",
    )
    parser.add_argument(
        "--output",
        default="/tmp/conveyor_failure_analysis.md",
        help="Output file path (default: /tmp/conveyor_failure_analysis.md)",
    )
    parser.add_argument(
        "--pipeline",
        default=None,
        help="Pipeline name to filter by. Default: auto-detect blocking pipeline.",
    )
    parser.add_argument(
        "--sample",
        type=int,
        default=2,
        help="Number of failures per category to deep-dive for logs (default: 2, 0 to skip)",
    )
    parser.add_argument(
        "--owner",
        default=None,
        help="Filter to nodes owned by this group (e.g., fboss_optics_phy). "
        "Uses the conveyor config 'Owner <name> Tests' groups.",
    )
    parser.add_argument(
        "--investigate",
        action="store_true",
        help="Deep-investigate test failures: fetch full stdout from everpaste, "
        "extract device hostnames, assertion details, and fbpkg version changes.",
    )
    parser.add_argument(
        "--all-nodes",
        action="store_true",
        help="Include all nodes (don't filter to blocking pipeline)",
    )
    args = parser.parse_args()

    print(f"Fetching {args.limit} releases for {args.conveyor_id}...")
    data = fetch_releases(args.conveyor_id, args.limit)
    print(f"  Got {len(data)} release items")

    # Determine blocking nodes
    blocking_nodes = None
    if not args.all_nodes:
        blocking_nodes = determine_blocking_nodes(data)
        print(f"  Detected {len(blocking_nodes)} blocking pipeline nodes")

    # Apply owner filter
    owner_nodes = None
    if args.owner:
        print(f"  Fetching owner group '{args.owner}'...")
        owner_nodes = fetch_owner_nodes(args.conveyor_id, args.owner)
        if owner_nodes:
            print(f"  Owner group has {len(owner_nodes)} nodes")
            if blocking_nodes is not None:
                blocking_nodes = blocking_nodes & owner_nodes
            else:
                blocking_nodes = owner_nodes
            print(f"  After intersection: {len(blocking_nodes)} nodes")
        else:
            print(f"  Warning: owner group '{args.owner}' not found, showing all nodes")

    # Extract failures
    failures = extract_failures(data, blocking_nodes)
    print(f"  Found {len(failures)} failures")

    if not failures:
        print("No failures found. Nothing to report.")
        return

    # Determine releases analyzed
    all_releases = set()
    for item in data:
        runs = item.get("runs") or {}
        for run in runs.values():
            if run and run.get("release_number"):
                all_releases.add(run["release_number"])

    # Classify
    categories = classify_failures(failures)
    print(f"  Classified into {len(categories)} categories")

    # Deep-dive into test logs
    if args.sample > 0:
        test_categories = [c for c in categories if c["type"] == "TEST FAILURE"]
        if test_categories:
            print(
                f"  Deep-diving into {len(test_categories)} test failure categories "
                f"(sampling {args.sample} per category)..."
            )
            for cat in test_categories:
                deep_dive_category(cat, args.sample)
                sig_preview = cat.get("log_signature", "")[:80]
                if sig_preview:
                    print(f"    {cat['name']}: found signature")

    # Investigation mode
    if args.investigate:
        test_categories = [c for c in categories if c["type"] == "TEST FAILURE"]
        if test_categories:
            print(f"  Investigating {len(test_categories)} test failure categories...")
            for cat in test_categories:
                investigate_category(cat, failures)
                inv = cat.get("investigation", {})
                devices = inv.get("devices", [])
                if devices:
                    print(f"    {cat['name']}: {len(set(devices))} device(s)")

    # Generate report
    report = generate_report(
        args.conveyor_id,
        categories,
        failures,
        sorted(all_releases),
        owner=args.owner,
        num_owner_nodes=len(owner_nodes) if owner_nodes else None,
    )

    with open(args.output, "w") as f:
        f.write(report)

    print(f"\nReport written to {args.output}")

    # Print summary to stdout
    print(f"\n{'=' * 60}")
    print(f"SUMMARY: {len(failures)} failures across {len(all_releases)} releases")
    print(f"{'=' * 60}")
    status_counts = Counter(f["status"] for f in failures)
    for status, count in status_counts.most_common():
        print(f"  {status}: {count}")
    print()
    print("Top categories:")
    for i, cat in enumerate(categories[:5]):
        letter = chr(ord("A") + i)
        print(f"  {letter}. {cat['name']} ({len(cat['failures'])}x, {cat['type']})")


if __name__ == "__main__":
    main()
