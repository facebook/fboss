#!/usr/bin/env python3
"""Systematically annotate BUCK files with # bazelify: exclude annotations.

Compares source files referenced in BUCK files against the cmake build to
determine which files, targets, or entire BUCK files should be excluded
from the Bazel build.

Uses full paths (relative to repo root) to avoid basename collisions
between files like fboss/agent/hw/bcm/tests/HwTestAclUtils.cpp and
fboss/agent/hw/sai/hw_test/HwTestAclUtils.cpp.
"""

from __future__ import annotations

import re
from pathlib import Path


def find_cmake_files(repo_root: Path) -> set[str]:
    """Extract all source file paths referenced in cmake/ files.

    Returns paths relative to repo root, e.g.:
    'fboss/agent/hw/sai/hw_test/HwTestAclUtils.cpp'
    """
    cmake_files = set()
    cmake_dir = repo_root / "cmake"
    if not cmake_dir.exists():
        return cmake_files

    for cmake_file in cmake_dir.rglob("*.cmake"):
        content = cmake_file.read_text()
        # Match lines like "  fboss/agent/foo/Bar.cpp"
        for match in re.finditer(r"^\s*(fboss/\S+\.(?:cpp|h))", content, re.MULTILINE):
            cmake_files.add(match.group(1))

    # Also check CMakeLists.txt at root
    cmakelists = repo_root / "CMakeLists.txt"
    if cmakelists.exists():
        content = cmakelists.read_text()
        for match in re.finditer(r"^\s*(fboss/\S+\.(?:cpp|h))", content, re.MULTILINE):
            cmake_files.add(match.group(1))
        for match in re.finditer(
            r"^\s*(common/\S+\.(?:cpp|h|thrift))", content, re.MULTILINE
        ):
            cmake_files.add(match.group(1))

    return cmake_files


def parse_buck_srcs(buck_file: Path, buck_dir: str) -> list[dict]:  # noqa: PLR0912, PLR0915
    """Parse targets and their source files from a BUCK file.

    Returns list of dicts with keys:
    - name: target name
    - rule_type: cpp_library, cpp_unittest, etc.
    - srcs: list of full paths (relative to repo root)
    - line: line number of target definition
    - already_excluded: bool - has # bazelify: exclude before it
    - inline_excludes: list of src files already marked inline
    """
    content = buck_file.read_text()
    lines = content.split("\n")
    targets = []

    # Check for file-level exclude-all
    for line in lines:
        stripped = line.strip()
        if (
            not stripped
            or stripped.startswith("load(")
            or stripped.startswith("oncall(")
        ):
            continue
        if stripped == "# bazelify: exclude-all":
            return []  # entire file excluded
        break

    # Find targets
    i = 0
    while i < len(lines):
        line = lines[i]

        # Check for target definition
        rule_match = re.match(
            r"(cpp_library|cpp_binary|cpp_unittest|cpp_benchmark)\(", line.strip()
        )
        if not rule_match:
            i += 1
            continue

        rule_type = rule_match.group(1)
        target_line = i

        # Check if already excluded at target level
        already_excluded = False
        for j in range(max(0, i - 3), i):
            if lines[j].strip() == "# bazelify: exclude":
                already_excluded = True
                break

        # Extract target block
        depth = 0
        block_lines = []
        j = i
        while j < len(lines):
            block_lines.append(lines[j])
            depth += lines[j].count("(") - lines[j].count(")")
            if depth <= 0 and j > i:
                break
            j += 1

        block = "\n".join(block_lines)

        # Extract name
        name_match = re.search(r'name\s*=\s*"([^"]+)"', block)
        if not name_match:
            i = j + 1
            continue
        name = name_match.group(1)

        # Extract srcs (only from srcs = [...] blocks)
        srcs = []
        inline_excludes = []
        srcs_match = re.search(r"srcs\s*=\s*\[(.*?)\]", block, re.DOTALL)
        if srcs_match:
            srcs_content = srcs_match.group(1)
            for src_match in re.finditer(r'"([^"]+\.cpp)"(.*)', srcs_content):
                filename = src_match.group(1)
                rest = src_match.group(2)
                full_path = f"{buck_dir}/{filename}"
                is_excluded = "# bazelify: exclude" in rest
                srcs.append(full_path)
                if is_excluded:
                    inline_excludes.append(full_path)

        targets.append(
            {
                "name": name,
                "rule_type": rule_type,
                "srcs": srcs,
                "line": target_line,
                "already_excluded": already_excluded,
                "inline_excludes": inline_excludes,
            }
        )

        i = j + 1

    return targets


def analyze_and_annotate(repo_root: Path, dry_run: bool = False):  # noqa: PLR0912, PLR0915
    """Main analysis: compare BUCK srcs against cmake and annotate."""
    cmake_files = find_cmake_files(repo_root)
    print(f"Found {len(cmake_files)} source files in cmake build")

    skip_dirs = {"bgp++", "broadcom-sai-sdk", "bazel", ".build_dir", "buck2"}

    stats = {
        "files_checked": 0,
        "exclude_all_files": 0,
        "exclude_targets": 0,
        "exclude_srcs": 0,
        "already_correct": 0,
    }

    for buck_file in sorted(repo_root.rglob("BUCK")):
        rel = buck_file.relative_to(repo_root)
        if any(part in skip_dirs for part in rel.parts):
            continue

        buck_dir = str(rel.parent)

        # Directory-level check: if no .cpp file under this directory appears
        # in cmake at all, the entire BUCK file should be exclude-all.
        # This catches cases where targets have no srcs (header-only, variables).
        # Skip this check for directories with .thrift files (cmake references
        # generated paths, not source .thrift paths) or hand-written BUILD files.
        dir_prefix = buck_dir + "/"
        dir_has_cmake_files = any(f.startswith(dir_prefix) for f in cmake_files)
        has_thrift = any(
            (buck_file.parent / f).suffix == ".thrift"
            for f in buck_file.parent.iterdir()
            if f.is_file()
        )
        has_handwritten_build = (buck_file.parent / "BUILD.bazel").exists() and not (
            buck_file.parent / "BUILD.bazel"
        ).read_text().startswith("# AUTO-GENERATED")
        is_fboss_dir = buck_dir.startswith("fboss/")
        if (
            not dir_has_cmake_files
            and not has_thrift
            and not has_handwritten_build
            and is_fboss_dir
        ):
            content = buck_file.read_text()
            if "# bazelify: exclude-all" not in content:
                stats["exclude_all_files"] += 1
                if not dry_run:
                    _apply_exclude_all(buck_file)
                else:
                    print(f"EXCLUDE-ALL (no cmake files in dir): {buck_file}")
            continue

        targets = parse_buck_srcs(buck_file, buck_dir)
        if not targets:
            continue

        stats["files_checked"] += 1

        # For each target, check which srcs are in cmake
        changes = []

        for target in targets:
            if target["already_excluded"]:
                continue

            srcs_in_cmake = []
            srcs_not_in_cmake = []

            for src in target["srcs"]:
                # Skip facebook/ files (always excluded by converter)
                if "/facebook/" in src:
                    continue
                if src in cmake_files:
                    srcs_in_cmake.append(src)
                else:
                    srcs_not_in_cmake.append(src)

            if not srcs_in_cmake and not srcs_not_in_cmake:
                # No non-facebook srcs at all - target is facebook-only
                continue

            if not srcs_in_cmake:
                # ALL srcs are not in cmake - exclude entire target
                already_inline = set(target["inline_excludes"])
                if len(already_inline) == len(target["srcs"]):
                    stats["already_correct"] += 1
                else:
                    changes.append(("target", target))
                    stats["exclude_targets"] += 1
            else:
                # Some srcs are in cmake, some not - inline exclude the non-cmake ones
                for src in srcs_not_in_cmake:
                    if src not in target["inline_excludes"]:
                        changes.append(("src", target, src))
                        stats["exclude_srcs"] += 1

        if not changes:
            continue

        # Check if ALL targets should be excluded -> file-level exclude-all
        all_excluded = all(
            t["already_excluded"]
            or any(c[0] == "target" and c[1] is t for c in changes)
            for t in targets
        )

        if all_excluded and len(targets) > 1:
            stats["exclude_all_files"] += 1
            if not dry_run:
                _apply_exclude_all(buck_file)
            else:
                print(f"EXCLUDE-ALL: {buck_file}")
            continue

        if not dry_run:
            _apply_changes(buck_file, buck_dir, changes)
        else:
            for change in changes:
                if change[0] == "target":
                    print(f"EXCLUDE TARGET: {buck_file}:{change[1]['name']}")
                else:
                    src = change[2]
                    print(f"EXCLUDE SRC: {buck_file}:{change[1]['name']} -> {src}")

    print("\nSummary:")
    print(f"  BUCK files checked: {stats['files_checked']}")
    print(f"  Files to exclude-all: {stats['exclude_all_files']}")
    print(f"  Targets to exclude: {stats['exclude_targets']}")
    print(f"  Individual srcs to exclude: {stats['exclude_srcs']}")
    print(f"  Already correct: {stats['already_correct']}")


def _apply_exclude_all(buck_file: Path):
    """Add # bazelify: exclude-all to a BUCK file.

    Existing per-target or inline annotations are left in place — they're
    harmless under exclude-all and removing them risks corrupting the file.
    """
    content = buck_file.read_text()
    if "# bazelify: exclude-all" in content:
        return  # already has it
    lines = content.split("\n")
    insert_idx = 0
    for j, line in enumerate(lines):
        stripped = line.strip()
        if (
            stripped.startswith("load(")
            or stripped.startswith("oncall(")
            or not stripped
        ):
            insert_idx = j + 1
        else:
            break
    lines.insert(insert_idx, "\n# bazelify: exclude-all\n")
    buck_file.write_text("\n".join(lines))


def _apply_changes(buck_file: Path, buck_dir: str, changes):
    """Apply target-level and inline exclude annotations."""
    content = buck_file.read_text()
    lines = content.split("\n")

    # Collect targets to exclude and srcs to exclude
    targets_to_exclude = set()
    srcs_to_exclude = set()
    for change in changes:
        if change[0] == "target":
            targets_to_exclude.add(change[1]["name"])
        else:
            srcs_to_exclude.add(change[2])

    new_lines = []
    for i, line in enumerate(lines):
        # Check if this line starts a target that should be excluded
        rule_match = re.match(
            r"(cpp_library|cpp_binary|cpp_unittest|cpp_benchmark)\(", line.strip()
        )
        if rule_match:
            # Check next few lines for the name
            block = "\n".join(lines[i : min(i + 5, len(lines))])
            name_match = re.search(r'name\s*=\s*"([^"]+)"', block)
            if (
                name_match
                and name_match.group(1) in targets_to_exclude
                and not (new_lines and new_lines[-1].strip() == "# bazelify: exclude")
            ):
                new_lines.append("# bazelify: exclude")

        # Check if this line has a src that should be excluded
        src_match = re.search(r'"([^"]+\.cpp)"', line)
        if src_match and "# bazelify: exclude" not in line:
            filename = src_match.group(1)
            full_path = f"{buck_dir}/{filename}"
            if full_path in srcs_to_exclude:
                line = line.rstrip() + "  # bazelify: exclude"  # noqa: PLW2901

        new_lines.append(line)

    buck_file.write_text("\n".join(new_lines))


def generate_excludes(repo_root: Path, dry_run: bool = False):
    """Entry point for bazelify.py --generate-excludes."""
    analyze_and_annotate(repo_root, dry_run=dry_run)
