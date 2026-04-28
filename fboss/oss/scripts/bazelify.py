#!/usr/bin/env python3
"""
Convert BUCK files to Bazel BUILD files for FBOSS OSS build.

This script:
1. Parses all BUCK files to extract targets and their attributes
2. Translates Buck rules to Bazel equivalents (cpp_library -> cc_library, etc.)
3. Resolves dependencies (intra-FBOSS, third-party, external)
4. Auto-discovers pre-built third-party deps from the getdeps install dir
5. Generates BUILD.bazel files (one per BUCK file directory)
6. Generates MODULE.bazel with pre-built third-party deps

Generated BUILD.bazel files are never committed to git.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import subprocess
import sys
import textwrap
from dataclasses import dataclass, field
from pathlib import Path

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

# Default getdeps install directory
DEFAULT_INSTALL_DIR = "/var/FBOSS/tmp_bld_dir/installed"

# Meta-internal dependency prefixes that don't exist in OSS.
# Targets referencing these will have the deps stripped (with a warning).
INTERNAL_DEP_PREFIXES = [
    "//analytics/",
    "//common/base/",
    "//common/config/",
    "//common/encode/",
    "//common/encrypt/",
    "//common/fbwhoami:",
    "//common/http_client/",
    "//common/init:",
    "//common/services/",
    "//common/smc/",
    "//common/strings/",
    "//common/thrift/cpp/server/",
    # "//configerator/" is partially available - individual deps checked
    "//dba/",
    "//employee/",
    "//groups/",
    "//infrasec/",
    "//libfb/",
    "//logging/",
    "//monitoring/",
    "//neteng/",
    "//nettools/",
    "//openr/",
    "//schedulers/",
    "//scribe/",
    "//scuba/",
    "//security/",
    "//servicerouter/",
    "//task/",
    "//thrift/annotation:",
    "//tupperware/",
]

# Buck load() paths that indicate Meta-internal macros.
# Targets using macros from these paths are skipped.
INTERNAL_LOAD_PREFIXES = [
    "@fbcode_macros//",
    "@fbsource//tools/",
    "//fboss/thriftpath_plugin/facebook:",
    "//fboss/build:",
    "//fbpkg:",
    "//folly/docs:",
]

# Mapping from Buck external dep paths to Bazel labels.
# All sub-targets under the same prefix map to the same Bazel dep.
EXTERNAL_DEP_MAP = {
    "//folly": "@folly//:folly",
    "//fb303": "@fb303//:fb303",
    "//thrift": "@fbthrift//:fbthrift",
    "//fatal": "@fatal//:fatal",
    "//wangle": "@wangle//:wangle",
    "//fizz": "@fizz//:fizz",
    "//mvfst": "@mvfst//:mvfst",
    "//common/time": "@folly//:folly",  # Time utilities often in folly
}

# Mapping from exported_external_deps / fbsource third-party to Bazel labels
THIRD_PARTY_MAP = {
    "CLI11": "@CLI11//:CLI11",
    "boost": "@boost//:boost",
    "gflags": "@gflags//:gflags",
    "glog": "@glog//:glog",
    "fmt": "@fmt//:fmt",
    "re2": "@re2//:re2",
    "zstd": "@zstd//:zstd",
    "snappy": "@snappy//:snappy",
    "lz4": "@lz4//:lz4",
    "libsodium": "@libsodium//:libsodium",
    "libyaml": "@libyaml//:libyaml",
    "yaml-cpp": "@yaml_cpp//:yaml_cpp",
    "openssl": "@openssl//:openssl",
    "curl": "@libcurl//:libcurl",
    "libcurl": "@libcurl//:libcurl",
    "libusb": "@libusb//:libusb",
    "libnl": "@libnl//:libnl",
    "libgpiod": "@libgpiod//:libgpiod",
    "iproute2": "@iproute2//:iproute2",
    "range-v3": "@range_v3//:range_v3",
    "nlohmann-json": "@nlohmann_json//:nlohmann_json",
    "tabulate": "@tabulate//:tabulate",
    "tabulatecpp": "@tabulate//:tabulate",
    "exprtk": "@exprtk//:exprtk",
    "nlohmann_json": "@nlohmann_json//:nlohmann_json",
    "systemd": "@systemd//:systemd",
    "libiberty": "@libiberty//:libiberty",
    "sai": "@libsai//:libsai",
    "broadcom-xgs-robo": "@sai_impl//:sai_impl",
}

# Mapping from fbsource//third-party/* to Bazel labels
FBSOURCE_THIRD_PARTY_MAP = {
    "fbsource//third-party/glog": "@glog//:glog",
    "fbsource//third-party/fmt": "@fmt//:fmt",
    "fbsource//third-party/googletest": "@googletest//:googletest",
    "fbsource//third-party/gflags": "@gflags//:gflags",
    "fbsource//third-party/boost": "@boost//:boost",
    "fbsource//third-party/re2": "@re2//:re2",
    "fbsource//third-party/yaml-cpp": "@yaml_cpp//:yaml_cpp",
    "fbsource//third-party/range-v3": "@range_v3//:range_v3",
    "fbsource//third-party/exprtk": "@exprtk//:exprtk",
    "fbsource//third-party/libpcap": None,
}

# Thrift sub-target suffixes that should map back to the base thrift target
THRIFT_SUFFIXES = [
    "-cpp2-types",
    "-cpp2-services",
    "-cpp2-clients",
    "-cpp2-visitation",
    "-cpp2-metadata",
    "-cpp2-serialization",
    "-cpp2-thriftpath",
    "-cpp2",
    "-py",
    "-py3",
    "-rust",
    "-go",
]


def _load_skip_dirs(repo_root: Path) -> set[str]:
    """Load ignored path components from .gitignore and .git/info/exclude."""
    skip: set[str] = set()
    for path in (repo_root / ".gitignore", repo_root / ".git" / "info" / "exclude"):
        try:
            for raw in path.read_text().splitlines():
                line = raw.strip()
                if line and not line.startswith("#"):
                    skip.add(line.strip("/"))
        except FileNotFoundError:
            pass
    return skip


SKIP_DIRS = _load_skip_dirs(Path(__file__).resolve().parents[3])

# Versions of Bazel module dependencies embedded in generated MODULE.bazel.
# Bump these here when upgrading.
_RULES_CC_VERSION = "0.2.17"
_RULES_PYTHON_VERSION = "1.0.0"
# Hedron compile-commands extractor — pinned to a specific commit (not in BCR).
# To upgrade: update the commit hash and run
# `bazel run //fboss/oss/refresh:refresh_compile_commands`.
_HEDRON_COMMIT = "abb61a688167623088f8768cc9264798df6a9d10"


# ---------------------------------------------------------------------------
# Data structures
# ---------------------------------------------------------------------------


@dataclass
class BuckTarget:
    """Represents a parsed Buck target."""

    name: str
    rule_type: str
    buck_path: str  # e.g., "//fboss/agent"
    srcs: list[str] = field(default_factory=list)
    headers: list[str] = field(default_factory=list)
    deps: list[str] = field(default_factory=list)
    exported_deps: list[str] = field(default_factory=list)
    external_deps: list[str] = field(default_factory=list)
    exported_external_deps: list[str] = field(default_factory=list)
    compiler_flags: list[str] = field(default_factory=list)
    preprocessor_flags: list[str] = field(default_factory=list)
    propagated_pp_flags: list[str] = field(default_factory=list)
    thrift_srcs: dict[str, list[str]] = field(default_factory=dict)
    thrift_cpp2_options: str = ""
    cpp2_deps: list[str] = field(default_factory=list)
    languages: list[str] = field(default_factory=list)
    link_whole: bool = False
    resources: list[str] = field(default_factory=list)
    size: str = ""
    annotations: dict[str, list[str]] = field(default_factory=dict)
    file_path: str = ""
    # custom_rule fields
    build_script_dep: str = ""
    build_args: str = ""
    output_gen_files: list[str] = field(default_factory=list)


def _find_repo_files(repo_root: Path, file_predicates: list[str]) -> list[Path]:
    """Find repo files using ``find`` with common directory pruning.

    Restricts the search to the ``fboss/``, ``build/``, and ``common/``
    subtrees.  Prunes hidden directories, ``bazel-*`` output symlinks, and
    ``__pycache__``.  Returns absolute paths sorted for determinism (~40ms vs
    ~2s for Python rglob on a large repo).
    """
    search_dirs = [
        str(d)
        for d in [repo_root / "fboss", repo_root / "build", repo_root / "common"]
        if d.is_dir()
    ] or [str(repo_root)]
    find_args = [
        "find", *search_dirs,
        "(", "-name", ".*", "-o", "-name", "bazel-*", "-o", "-name", "__pycache__", ")", "-prune",
        "-o",
        "(", *file_predicates, ")",
        "-type", "f", "-print",
    ]  # fmt: skip
    result = subprocess.run(find_args, check=False, capture_output=True, text=True)
    return sorted(Path(line) for line in result.stdout.splitlines() if line)


# ---------------------------------------------------------------------------
# BUCK Parser
# ---------------------------------------------------------------------------


class BuckParser:
    """Parse BUCK files and extract target definitions."""

    # Rule types we care about
    RULE_TYPES = {  # noqa: RUF012
        "cpp_library",
        "cpp_binary",
        "cpp_unittest",
        "cpp_benchmark",
        "thrift_library",
        "custom_rule",
        "python_binary",
        "python_library",
        "python_unittest",
        "export_file",
        "buck_filegroup",
    }

    def __init__(self, repo_root: str):
        self.repo_root = Path(repo_root)
        self.targets: dict[str, list[BuckTarget]] = {}  # buck_path -> targets
        self.all_targets: dict[str, BuckTarget] = {}  # full_name -> target
        self._skipped_macros: set[str] = set()
        # Packages whose BUCK files had # bazelify: exclude-all (cpp targets
        # were excluded but thrift targets are still parsed).  These packages
        # must NOT get auto-generated _impl cc_library targets even if .cpp
        # files exist in their directory.
        self.exclude_all_pkgs: set[str] = set()
        # Packages with # bazelify: include <file.bzl>:<func> annotations.
        # Maps buck_path -> list of (bzl_file, func_name) tuples.
        self._bzl_includes: dict[str, list[tuple[str, str]]] = {}
        # Target names that were explicitly excluded via # bazelify: exclude.
        self._excluded_target_names: set[str] = set()

    def parse_all(self) -> dict[str, list[BuckTarget]]:
        """Parse all BUCK files in the repository."""
        for buck_file in _find_repo_files(self.repo_root, ["-name", "BUCK"]):
            self._parse_file(buck_file)
        return self.targets

    def _parse_file(self, file_path: Path):  # noqa: PLR0912, PLR0915
        """Parse a single BUCK file."""
        try:
            content = file_path.read_text()
        except Exception as e:
            print(f"Warning: Could not read {file_path}: {e}", file=sys.stderr)
            return

        rel_path = file_path.parent.relative_to(self.repo_root)
        buck_path = "//" + str(rel_path).replace("\\", "/")

        # Scan for file-level bazelify annotations:
        #   # bazelify: exclude-all  — skip all C++ targets (keep thrift)
        #   # bazelify: include <file.bzl>:<func>  — load and call a .bzl
        #     macro in the generated BUILD.bazel (Bazel-native include)
        exclude_all_cpp = False
        bzl_includes: list[tuple[str, str]] = []
        for line in content.split("\n"):
            stripped = line.strip()
            if not stripped.startswith("#"):
                continue
            if stripped == "# bazelify: exclude-all":
                exclude_all_cpp = True
                self.exclude_all_pkgs.add(buck_path)
            m = re.match(r"#\s*bazelify:\s*include\s+(\S+\.bzl):(\w+)", stripped)
            if m:
                bzl_includes.append((m.group(1), m.group(2)))
        if bzl_includes:
            self._bzl_includes[buck_path] = bzl_includes

        # Detect internal-only macros from load() statements
        internal_macros = self._detect_internal_macros(content)

        # Parse annotations (comments before targets)
        annotations_by_line = self._parse_annotations(content)

        # Find all rule invocations
        targets_in_file = []
        for rule_type in self.RULE_TYPES:
            # When exclude_all_cpp is set, only process thrift/export rules
            if exclude_all_cpp and rule_type not in (
                "thrift_library",
                "export_file",
                "buck_filegroup",
            ):
                continue
            pattern = rf"(?:^|\n)({rule_type})\s*\("
            for match in re.finditer(pattern, content):
                paren_pos = content.find("(", match.start())
                block = self._extract_block(content, paren_pos)
                if not block:
                    continue

                # Get target name
                name_match = re.search(r'name\s*=\s*"([^"]+)"', block)
                if not name_match:
                    continue
                name = name_match.group(1)

                # Check for annotations on lines before this target.
                # match.start() is the position of the \n before the rule
                # keyword (from the (?:^|\n) group), so we use match.start(1)
                # (the rule keyword itself) for an accurate line number.
                # Scan backwards from the target, collecting annotations from
                # the contiguous comment/blank block immediately above it.
                # Stop at the first line that is neither a comment nor blank
                # to avoid bleeding annotations from unrelated targets.
                rule_kw_pos = match.start(1)
                line_num = content[:rule_kw_pos].count("\n")
                all_lines = content.split("\n")
                annotations: dict[str, list[str]] = {}
                for i in range(line_num, max(0, line_num - 50) - 1, -1):
                    if i in annotations_by_line:
                        for k, v_list in annotations_by_line[i].items():
                            annotations.setdefault(k, []).extend(v_list)
                    elif i < line_num:
                        # Not an annotation line - check if comment or blank
                        line_text = all_lines[i].strip() if i < len(all_lines) else ""
                        if not line_text or line_text.startswith("#"):
                            continue  # blank or comment, keep scanning
                        break  # code line - stop scanning

                # Skip if annotated
                if "exclude" in annotations:
                    self._excluded_target_names.add(name)
                    continue

                target = self._parse_target(
                    rule_type, name, block, buck_path, str(file_path), annotations
                )
                targets_in_file.append(target)
                self.all_targets[f"{buck_path}:{name}"] = target

        # Also find custom macros that we should skip
        for macro_name in internal_macros:
            pattern = rf"(?:^|\n){re.escape(macro_name)}\s*\("
            if re.search(pattern, content):
                self._skipped_macros.add(macro_name)

        if targets_in_file:
            self.targets[buck_path] = targets_in_file

    def _detect_internal_macros(self, content: str) -> set[str]:
        """Detect macros loaded from internal-only .bzl files."""
        internal_macros = set()
        for match in re.finditer(r'load\("([^"]+)"(?:,\s*"([^"]+)")*\)', content):
            bzl_path = match.group(1)
            # Check if this is an internal load path
            is_internal = any(bzl_path.startswith(p) for p in INTERNAL_LOAD_PREFIXES)
            if not is_internal and bzl_path.startswith("//"):
                # Check if the .bzl file actually exists on disk
                local_path = self.repo_root / bzl_path.lstrip("/").replace(":", "/")
                if not local_path.exists():
                    is_internal = True

            if is_internal:
                # Extract all imported symbols.  Use [^"]+ so the path string
                # (which contains @, /, . and :) is also matched; then [1:]
                # correctly skips it and leaves only the symbol names.
                symbols = re.findall(r'"([^"]+)"', match.group(0))
                for sym in symbols[1:]:  # skip the path itself
                    # Don't mark standard build rules as internal macros
                    if sym not in self.RULE_TYPES:
                        internal_macros.add(sym)
        return internal_macros

    def _parse_annotations(self, content: str) -> dict[int, dict[str, list[str]]]:
        """Parse # bazelify: annotations from comments."""
        annotations: dict[int, dict[str, list[str]]] = {}
        for i, raw_line in enumerate(content.split("\n")):
            stripped = raw_line.strip()
            for prefix in (r"#\s*bazelify:",):
                m = re.match(prefix + r"\s*(\w+)(?:\s*=\s*(.+))?", stripped)
                if m:
                    key = m.group(1)
                    val = (m.group(2) or "true").strip()
                    if i not in annotations:
                        annotations[i] = {}
                    annotations[i].setdefault(key, []).append(val)
        return annotations

    def _extract_block(self, content: str, paren_pos: int) -> str | None:  # noqa: PLR0912
        """Extract a balanced parenthesized block."""
        if paren_pos >= len(content) or content[paren_pos] != "(":
            return None
        depth = 1
        pos = paren_pos + 1
        while pos < len(content) and depth > 0:
            ch = content[pos]
            if ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
            elif ch == '"':
                # Detect triple-quoted strings before single-quoted strings.
                if content[pos : pos + 3] == '"""':
                    # Skip past the opening triple-quote
                    pos += 3
                    while pos < len(content):
                        if content[pos : pos + 3] == '"""':
                            pos += 3  # skip the closing triple-quote
                            break
                        if content[pos] == "\\":
                            pos += 1  # skip escaped character
                        pos += 1
                    continue  # pos already advanced; skip the pos += 1 below
                # Single-quoted string
                pos += 1
                while pos < len(content) and content[pos] != '"':
                    if content[pos] == "\\":
                        pos += 1
                    pos += 1
            elif ch == "#":
                # Skip comments
                while pos < len(content) and content[pos] != "\n":
                    pos += 1
            pos += 1
        if depth == 0:
            return content[paren_pos:pos]
        return None

    def _extract_string_list(self, block: str, key: str) -> list[str]:
        """Extract a list of strings for a given key.

        Uses a negative lookbehind for word characters so that a key like
        'deps' does not accidentally match 'exported_deps', while also
        handling the case where the key appears right after the opening
        parenthesis (no preceding comma or space).
        """
        # (?<![a-zA-Z_]) ensures the key is not preceded by a letter or
        # underscore, so 'deps' won't match inside 'exported_deps'.
        # (?:^|[\s,(]) covers start-of-string, whitespace, comma, and (.
        # Use a pattern that handles ] inside quoted strings correctly:
        # the list content is either a quoted string (which may contain [ or ])
        # or any non-bracket, non-quote character.
        pattern = (
            rf'(?:(?:^|[\s,(])(?<![a-zA-Z_]))({key})\s*=\s*\[((?:"[^"]*"|[^"\[\]])*)\]'
        )
        match = re.search(pattern, block, re.MULTILINE)
        if not match:
            return []
        list_content = match.group(2)
        # Filter out items with # bazelify: exclude on the same line
        result = []
        for item_match in re.finditer(r'"([^"]*)"', list_content):
            # Check if there's an # bazelify: exclude comment after this item on the same line
            end_pos = item_match.end()
            rest_of_line = list_content[end_pos:].split("\n", 1)[0]
            if "# bazelify: exclude" in rest_of_line:
                continue
            result.append(item_match.group(1))
        return result

    def _extract_string(self, block: str, key: str) -> str:
        """Extract a single string value for a given key."""
        pattern = rf'(?:^|[\s,\n]){key}\s*=\s*"([^"]*)"'
        match = re.search(pattern, block, re.MULTILINE)
        return match.group(1) if match else ""

    def _extract_bool(self, block: str, key: str) -> bool | None:
        """Extract a boolean value for a given key."""
        pattern = rf"(?:^|[\s,\n]){key}\s*=\s*(True|False)"
        match = re.search(pattern, block, re.MULTILINE)
        if match:
            return match.group(1) == "True"
        return None

    def _extract_dict(self, block: str, key: str) -> dict[str, list[str]]:
        """Extract a dict value like thrift_srcs = {"file.thrift": [...]}."""
        pattern = rf"{key}\s*=\s*\{{([^}}]*)\}}"
        match = re.search(pattern, block, re.DOTALL)
        if not match:
            return {}
        dict_content = match.group(1)
        result = {}
        # Match "key": [...] (list form)
        for item_match in re.finditer(r'"([^"]+)"\s*:\s*\[([^\]]*)\]', dict_content):
            k = item_match.group(1)
            vals = re.findall(r'"([^"]*)"', item_match.group(2))
            result[k] = vals
        # Match "key": "value" (bare string form, e.g., thrift_srcs with service names)
        for item_match in re.finditer(r'"([^"]+)"\s*:\s*"([^"]*)"', dict_content):
            k = item_match.group(1)
            if k not in result:  # don't overwrite list matches
                result[k] = [item_match.group(2)]
        return result

    def _extract_external_deps(self, block: str, key: str) -> list[str]:
        """Extract external deps which can be strings or tuples."""
        pattern = rf"(?:^|[\s,\n])({key})\s*=\s*\[(.*?)\]"
        match = re.search(pattern, block, re.DOTALL | re.MULTILINE)
        if not match:
            return []
        list_content = match.group(2)
        result = []
        # Match 3-element tuple: ("pkg", None, "lib") or ("pkg", "ver", "lib")
        for tuple_match in re.finditer(
            r'\(\s*"([^"]+)"\s*,\s*(?:None|"[^"]*")\s*,\s*"([^"]+)"\s*\)', list_content
        ):
            pkg = tuple_match.group(1)
            lib = tuple_match.group(2)
            result.append(f"{pkg}:{lib}")
        # Match 2-element tuple: ("pkg", None) or ("pkg", "ver")
        for tuple_match in re.finditer(
            r'\(\s*"([^"]+)"\s*,\s*(?:None|"[^"]*")\s*\)(?!\s*,\s*")', list_content
        ):
            pkg = tuple_match.group(1)
            if pkg not in [r.split(":")[0] for r in result]:  # not already a 3-tuple
                result.append(pkg)
        # Match simple string form: "pkg"
        # But exclude strings already captured as part of tuples
        tuple_spans = set()
        for tuple_match in re.finditer(r"\([^)]+\)", list_content):
            tuple_spans.add((tuple_match.start(), tuple_match.end()))

        for str_match in re.finditer(r'"([^"]+)"', list_content):
            # Check if this string is inside a tuple
            pos = str_match.start()
            in_tuple = any(s <= pos <= e for s, e in tuple_spans)
            if not in_tuple:
                result.append(str_match.group(1))
        return result

    def _parse_target(
        self,
        rule_type: str,
        name: str,
        block: str,
        buck_path: str,
        file_path: str,
        annotations: dict[str, list[str]],
    ) -> BuckTarget:
        """Parse a target block into a BuckTarget."""
        # thrift_cpp2_options can be a string or list
        thrift_opts_str = self._extract_string(block, "thrift_cpp2_options")
        if not thrift_opts_str:
            opts_list = self._extract_string_list(block, "thrift_cpp2_options")
            thrift_opts_str = ",".join(opts_list)

        return BuckTarget(
            name=name,
            rule_type=rule_type,
            buck_path=buck_path,
            srcs=self._extract_string_list(block, "srcs"),
            headers=self._extract_string_list(block, "headers"),
            deps=self._extract_string_list(block, "deps"),
            exported_deps=self._extract_string_list(block, "exported_deps"),
            external_deps=self._extract_external_deps(block, "external_deps"),
            exported_external_deps=self._extract_external_deps(
                block, "exported_external_deps"
            ),
            compiler_flags=self._extract_string_list(block, "compiler_flags"),
            preprocessor_flags=self._extract_string_list(block, "preprocessor_flags"),
            propagated_pp_flags=self._extract_string_list(block, "propagated_pp_flags"),
            thrift_srcs=self._extract_dict(block, "thrift_srcs"),
            thrift_cpp2_options=thrift_opts_str,
            cpp2_deps=self._extract_string_list(block, "cpp2_deps"),
            languages=self._extract_string_list(block, "languages"),
            link_whole=self._extract_bool(block, "link_whole") or False,
            resources=self._extract_string_list(block, "resources"),
            size=self._extract_string(block, "size") or "",
            annotations=annotations,
            file_path=file_path,
            build_script_dep=self._extract_string(block, "build_script_dep"),
            build_args=self._extract_string(block, "build_args"),
            output_gen_files=self._extract_string_list(block, "output_gen_files"),
        )


# ---------------------------------------------------------------------------
# Third-party dependency discovery
# ---------------------------------------------------------------------------


@dataclass
class PrebuiltDep:
    """A pre-built third-party dependency."""

    name: str  # canonical name (e.g., "folly")
    path: str  # absolute path to installed dir
    lib_dir: str  # "lib" or "lib64"
    has_static_libs: bool
    has_shared_libs: bool
    static_libs: list[str] = field(default_factory=list)
    shared_libs: list[str] = field(default_factory=list)
    include_dir: str = ""  # path to include dir
    is_header_only: bool = False
    extra_include_dirs: list[str] = field(default_factory=list)  # e.g. ["experimental"]


def discover_prebuilt_deps(install_dir: str) -> dict[str, PrebuiltDep]:  # noqa: PLR0912
    """Scan the getdeps install directory and discover pre-built deps."""
    deps = {}
    install_path = Path(install_dir)
    if not install_path.exists():
        print(f"Warning: Install dir {install_dir} does not exist", file=sys.stderr)
        return deps

    for entry in sorted(install_path.iterdir()):
        if not entry.is_dir():
            continue
        name = entry.name

        # Strip getdeps hash suffix using the .built-by-getdeps marker file
        # which contains the exact hash, e.g. "PmWtKGYNYhX-yLkETvaFAYEJ53FZ0..."
        marker = entry / ".built-by-getdeps"
        if marker.exists():
            dep_hash = marker.read_text().strip()
            if name.endswith("-" + dep_hash):
                canonical = name[: -(len(dep_hash) + 1)]
            else:
                canonical = name
        else:
            canonical = name

        # Skip the fboss build itself
        if canonical == "fboss":
            continue

        # Check for lib/ or lib64/
        lib_dir = ""
        if (entry / "lib").is_dir():
            lib_dir = "lib"
        elif (entry / "lib64").is_dir():
            lib_dir = "lib64"

        # Check for include/
        include_dir = ""
        if (entry / "include").is_dir():
            include_dir = "include"

        # Special case: fatal is header-only with headers directly in fatal/
        if not include_dir:
            # Check for headers at root level (e.g., fatal/, exprtk.hpp)
            has_root_headers = (
                any(entry.glob("*.h"))
                or any(entry.glob("*.hpp"))
                or any(
                    d.is_dir() and any(d.glob("*.h"))
                    for d in entry.iterdir()
                    if d.is_dir() and d.name not in ("lib", "lib64", "share", "cmake")
                )
            )
            if has_root_headers:
                include_dir = "."

        # Discover libraries
        static_libs = []
        shared_libs = []
        if lib_dir:
            lib_path = entry / lib_dir
            for f in sorted(lib_path.iterdir()):
                if f.suffix == ".a":
                    static_libs.append(f.name)
                elif f.suffix == ".so":
                    shared_libs.append(f.name)

        is_header_only = not static_libs and not shared_libs and bool(include_dir)

        # Only keep the newest version if multiple exist for same canonical name
        if canonical in deps:
            # Keep the one with more recent mtime
            existing = deps[canonical]
            if entry.stat().st_mtime > Path(existing.path).stat().st_mtime:
                pass  # will overwrite below
            else:
                continue

        deps[canonical] = PrebuiltDep(
            name=canonical,
            path=str(entry),
            lib_dir=lib_dir,
            has_static_libs=bool(static_libs),
            has_shared_libs=bool(shared_libs),
            static_libs=static_libs,
            shared_libs=shared_libs,
            include_dir=include_dir,
            is_header_only=is_header_only,
            extra_include_dirs=_parse_manifest_extra_includes(canonical)
            if is_header_only
            else [],
        )

    return deps


def generate_module_bazel(deps: dict[str, PrebuiltDep]) -> str:
    """Generate MODULE.bazel content with use_repo_rule for pre-built deps."""
    _indent = "        "  # shared indent for template content
    chunks = [
        f"""\
        # AUTO-GENERATED by bazelify.py - DO NOT EDIT
        # Third-party dependencies from getdeps.py pre-built libraries

        module(
            name = "fboss",
            version = "0.1",
        )

        bazel_dep(name = "rules_cc", version = "{_RULES_CC_VERSION}")
        single_version_override(
            module_name = "rules_cc",
            version = "{_RULES_CC_VERSION}",
            patches = [
                "//fboss/build_defs:disable_include_validation.patch",
            ],
        )

        # Import new_local_repository for pre-built deps
        new_local_repository = use_repo_rule(
            "@bazel_tools//tools/build_defs/repo:local.bzl",
            "new_local_repository",
        )
    """
    ]

    prebuilt_names = set(deps.keys())
    for name, dep in sorted(deps.items()):
        bazel_name = name.replace("-", "_").replace("+", "p")
        inter_deps = [
            f"@{n.replace('-', '_').replace('+', 'p')}//:{n.replace('-', '_').replace('+', 'p')}"
            for n in _parse_manifest_deps(name)
            if n in prebuilt_names
        ]
        build_content = _make_build_file_content(dep, bazel_name, inter_deps=inter_deps)
        if build_content is None:
            continue

        bc = textwrap.indent(build_content.strip(), _indent)
        chunks.append(f"""\
        new_local_repository(
            name = "{bazel_name}",
            path = "{dep.path}",
            build_file_content = \"\"\"\\
{bc}
        \"\"\",
        )
        """)

    for stub_name, stub_content in sorted(SYSTEM_LIB_STUBS.items()):
        if stub_name not in [d.name for d in deps.values()]:
            sc = textwrap.indent(stub_content, _indent)
            chunks.append(f"""\
        new_local_repository(
            name = "{stub_name}",
            path = "/usr",
            build_file_content = \"\"\"\\
        load("@rules_cc//cc:defs.bzl", "cc_library")
{sc}
        \"\"\",
        )
        """)

    return textwrap.dedent("\n".join(chunks))


def _synthetic_prebuilt_dep(name: str, manifest_info: dict) -> PrebuiltDep:
    """Create a synthetic PrebuiltDep from manifest info (no installed dir needed).

    This allows generating build_file_content before deps are actually built.
    The layout assumptions (lib vs lib64, static vs shared) are based on the
    manifest's builder type and cmake defines.
    """
    builder = manifest_info.get("builder", "cmake")
    is_header_only = builder == "nop" and name not in ("sai_impl", "iproute2")
    is_shared = name in PREFER_SHARED

    # Most cmake projects install to lib/, but some (glog, fmt on centos)
    # use lib64/.  Default to lib/ for synthetic deps.
    lib_dir = "lib"
    if name in ("glog", "googletest"):
        lib_dir = "lib64"

    return PrebuiltDep(
        name=name,
        path=f"/var/FBOSS/tmp_bld_dir/installed/{name}",  # placeholder path
        lib_dir=lib_dir,
        has_static_libs=not is_header_only and not is_shared,
        has_shared_libs=is_shared,
        static_libs=[],
        shared_libs=[f"lib{name}.so"] if is_shared else [],
        include_dir="include" if not is_header_only or name in ("libsai",) else ".",
        is_header_only=is_header_only,
        extra_include_dirs=_parse_manifest_extra_includes(name)
        if is_header_only
        else [],
    )


def _discover_getdeps_graph(
    scratch_path: str,
) -> list[dict]:
    """Discover the getdeps dependency graph by importing getdeps internals.

    Returns a list of dicts (in topological build order), each containing:
      name, deps, first_party, is_system, manifest_rel, revision_file_rel
    """
    import importlib  # noqa: PLC0415

    fbcode_builder_dir = str(
        Path(__file__).resolve().parents[3] / "build" / "fbcode_builder"
    )
    if fbcode_builder_dir not in sys.path:
        sys.path.insert(0, fbcode_builder_dir)

    buildopts = importlib.import_module("getdeps.buildopts")
    load_mod = importlib.import_module("getdeps.load")
    fetcher_mod = importlib.import_module("getdeps.fetcher")

    args = argparse.Namespace(
        scratch_path=scratch_path,
        install_prefix=None,
        host_type=None,
        facebook_internal=False,
        vcvars_path=None,
        allow_system_packages=True,
        lfs_path=None,
        shared_libs=False,
    )
    opts = buildopts.setup_build_options(args)
    ctx_gen = opts.get_context_generator()
    loader = load_mod.ManifestLoader(opts, ctx_gen)
    loader.load_manifest("fboss")
    all_deps = loader.manifests_in_dependency_order()

    repo_root = str(Path(__file__).resolve().parents[3])
    manifests_dir = opts.manifests_dir

    # Revision files for first-party projects
    rev_dir = Path(repo_root) / "build" / "deps" / "github_hashes"

    result = []
    for d in all_deps:
        if d.name == "fboss":
            continue
        ctx = ctx_gen.get_context(d.name)
        dep_list = d.get_dependencies(ctx)
        fetcher = loader.create_fetcher(d)
        is_sys = isinstance(fetcher, fetcher_mod.SystemPackageFetcher)
        is_first = d.is_first_party_project()

        manifest_rel = os.path.relpath(os.path.join(manifests_dir, d.name), repo_root)

        # Find revision file for first-party git projects
        rev_file_rel = None
        if is_first:
            for org_dir in rev_dir.iterdir():
                rev_path = org_dir / f"{d.name}-rev.txt"
                if rev_path.exists():
                    rev_file_rel = str(rev_path.relative_to(repo_root))
                    break

        builder = d.get("build", "builder", ctx=ctx) or "unknown"

        result.append(
            {
                "name": d.name,
                "deps": dep_list,
                "first_party": is_first,
                "is_system": is_sys,
                "manifest_rel": manifest_rel,
                "revision_file_rel": rev_file_rel,
                "builder": builder,
            }
        )

    return result


def _create_getdeps_build_files(repo_root: Path) -> None:
    """Create BUILD.bazel package markers and exports_files for getdeps inputs.

    These are needed so that manifest files and revision files can be
    referenced as Bazel labels (e.g., //build/fbcode_builder/manifests:folly).
    """
    # Package markers for intermediate directories
    for d in [
        "build",
        "build/fbcode_builder",
        "build/deps",
        "build/deps/github_hashes",
    ]:
        build_file = repo_root / d / "BUILD.bazel"
        if not build_file.exists():
            build_file.parent.mkdir(parents=True, exist_ok=True)
            build_file.write_text("# Package marker for getdeps inputs\n")

    # exports_files for manifests
    manifests_build = repo_root / "build/fbcode_builder/manifests/BUILD.bazel"
    manifests_build.write_text(
        '# AUTO-GENERATED by bazelify.py\nexports_files(glob(["*"]))\n'
    )

    # exports_files for revision files in each org directory
    rev_dir = repo_root / "build/deps/github_hashes"
    if rev_dir.is_dir():
        for org_dir in sorted(rev_dir.iterdir()):
            if org_dir.is_dir() and any(org_dir.glob("*-rev.txt")):
                build_file = org_dir / "BUILD.bazel"
                build_file.write_text(
                    "# AUTO-GENERATED by bazelify.py\n"
                    'exports_files(glob(["*-rev.txt"]))\n'
                )


def generate_module_bazel_getdeps(
    prebuilt_deps: dict[str, PrebuiltDep],
    getdeps_graph: list[dict],
    scratch_path: str,
    cache_url: str,
) -> str:
    """Generate MODULE.bazel using getdeps_repository for built deps.

    System deps and deps that are already pre-built use new_local_repository.
    Deps that need building use getdeps_repository.
    """
    _indent = "        "  # shared indent for template content
    chunks = [
        f"""\
        # AUTO-GENERATED by bazelify.py - DO NOT EDIT
        # Third-party dependencies built via getdeps_repository rules

        module(
            name = "fboss",
            version = "0.1",
        )

        bazel_dep(name = "rules_cc", version = "{_RULES_CC_VERSION}")
        single_version_override(
            module_name = "rules_cc",
            version = "{_RULES_CC_VERSION}",
            patches = [
                "//fboss/build_defs:disable_include_validation.patch",
            ],
        )

        # hedron_compile_commands generates compile_commands.json for clang
        # tooling (clang-tidy, clang-include-cleaner, clangd).  Not in BCR,
        # so pulled via archive_override.  Needs rules_python for its
        # py_binary target, plus a small patch to load py_binary from
        # rules_python (Bazel 9 removed native.py_binary).  See Hedron PR #278.
        # Run: bazel run //fboss/oss/refresh:refresh_compile_commands
        bazel_dep(name = "rules_python", version = "{_RULES_PYTHON_VERSION}")
        bazel_dep(name = "hedron_compile_commands", version = "0.0.0")
        archive_override(
            module_name = "hedron_compile_commands",
            urls = ["https://github.com/hedronvision/bazel-compile-commands-extractor/archive/{_HEDRON_COMMIT}.tar.gz"],
            strip_prefix = "bazel-compile-commands-extractor-{_HEDRON_COMMIT}",
            patches = [
                "//fboss/build_defs:hedron_module.patch",
                "//fboss/build_defs:hedron_build.patch",
                "//fboss/build_defs:hedron_refresh.patch",
                "//fboss/build_defs:hedron_refresh_py.patch",
                "//fboss/build_defs:hedron_action_cache_digest.patch",
            ],
            patch_strip = 1,
        )

        # Import rules for pre-built and getdeps-built deps
        new_local_repository = use_repo_rule(
            "@bazel_tools//tools/build_defs/repo:local.bzl",
            "new_local_repository",
        )
        getdeps_repository = use_repo_rule(
            "//fboss/build_defs:getdeps_dep.bzl",
            "getdeps_repository",
        )
    """
    ]

    # Build a set of deps that getdeps builds (not system, not fboss itself)
    getdeps_built = {d["name"] for d in getdeps_graph if not d["is_system"]}

    # For each dep in the getdeps graph (topological order)
    for dep_info in getdeps_graph:
        name = dep_info["name"]
        bazel_name = name.replace("-", "_").replace("+", "p")

        if dep_info["is_system"]:
            # System dep — skip, these are handled by system_lib_stubs
            # at the end (libusb, libnl, etc.) or are not referenced by
            # FBOSS BUCK files at all.
            continue

        # Built dep — use getdeps_repository
        # Generate build_file_content from prebuilt dep info if available,
        # otherwise create a synthetic PrebuiltDep from manifest metadata.
        # Filter dep_repos to only include deps that are also getdeps-built
        # (system deps don't become Bazel repos via getdeps_repository)
        dep_repos = [
            d.replace("-", "_").replace("+", "p")
            for d in dep_info["deps"]
            if d in getdeps_built
        ]
        inter_deps = [f"@{repo}//:{repo}" for repo in dep_repos]

        dep_obj = prebuilt_deps.get(name)
        if dep_obj is None:
            dep_obj = _synthetic_prebuilt_dep(name, dep_info)
        build_content = _make_build_file_content(
            dep_obj, bazel_name, inter_deps=inter_deps
        )
        if build_content is None:
            continue

        # Build optional attributes (indented 12 = 8 base + 4 inside rule)
        attr_indent = _indent + "    "
        oa = ""
        if dep_info["revision_file_rel"]:
            rev_label = "//" + dep_info["revision_file_rel"].replace(os.sep, "/")
            rev_parts = rev_label.rsplit("/", 1)
            rev_label = f"{rev_parts[0]}:{rev_parts[1]}"
            oa += f'{attr_indent}revision_file = "{rev_label}",\n'
        if dep_repos:
            oa += f"{attr_indent}dep_repos = {dep_repos},\n"
        if dep_info["first_party"]:
            oa += f"{attr_indent}first_party = True,\n"
        if scratch_path != "/var/FBOSS/tmp_bld_dir":
            oa += f'{attr_indent}scratch_path = "{scratch_path}",\n'
        if cache_url:
            oa += f'{attr_indent}cache_url = "{cache_url}",\n'

        bc = textwrap.indent(build_content.strip(), _indent)
        chunks.append(f"""\
        getdeps_repository(
            name = "{bazel_name}",
            project_name = "{name}",
            manifest = "//build/fbcode_builder/manifests:{name}",
{oa}\
            build_file_content = \"\"\"\\
{bc}
        \"\"\",
        )
        """)

    # System library stubs always emitted — they provide -l flags for
    # system-installed libraries that aren't built by getdeps.
    for stub_name, stub_content in sorted(SYSTEM_LIB_STUBS.items()):
        sc = textwrap.indent(stub_content, _indent)
        chunks.append(f"""\
        new_local_repository(
            name = "{stub_name}",
            path = "/usr",
            build_file_content = \"\"\"\\
        load("@rules_cc//cc:defs.bzl", "cc_library")
{sc}
        \"\"\",
        )
        """)

    return textwrap.dedent("\n".join(chunks))


# System library linkopts needed by pre-built deps at link time.
# Folly and fbthrift link against many system libs that aren't captured
# in the static archives. These must be passed as linker flags.
SYSTEM_LINKOPTS = {
    "folly": [
        "-levent",
        "-ldouble-conversion",
        "-lz",
        "-lssl",
        "-lcrypto",
        "-lbz2",
        "-llzma",
        "-llz4",
        "-lzstd",
        "-lsnappy",
        "-ldwarf",
        "-laio",
        "-lsodium",
        "/usr/lib64/libunwind.so",
        "-ldl",
        "-lpthread",
    ],
    "fbthrift": ["-lxxhash", "-lre2"],
    "fizz": ["-lre2"],
}


def _parse_manifest_deps(dep_name: str) -> list[str]:
    """Return the dependency names from a manifest's [dependencies] section."""
    import configparser  # noqa: PLC0415

    manifests_dir = (
        Path(__file__).resolve().parents[3] / "build" / "fbcode_builder" / "manifests"
    )
    path = manifests_dir / dep_name
    if not path.exists():
        return []
    cfg = configparser.ConfigParser(allow_no_value=True)
    cfg.read(str(path))
    if not cfg.has_section("dependencies"):
        return []
    return list(cfg.options("dependencies"))


def _parse_manifest_extra_includes(dep_name: str) -> list[str]:
    """Return install destination dirs (other than 'include') from [install.files].

    Used to detect deps like libsai that install headers into multiple
    directories (e.g. 'experimental') alongside the main 'include/' tree.
    """
    import configparser  # noqa: PLC0415

    manifests_dir = (
        Path(__file__).resolve().parents[3] / "build" / "fbcode_builder" / "manifests"
    )
    path = manifests_dir / dep_name
    if not path.exists():
        return []
    cfg = configparser.ConfigParser(allow_no_value=True)
    cfg.read(str(path))
    if not cfg.has_section("install.files"):
        return []
    # Format: src = dest; return dest values that aren't the primary include dir
    return [
        dest for _src, dest in cfg.items("install.files") if dest and dest != "include"
    ]


# No deps need shared linking in the Bazel build.  glog and gflags are
# built statically (BUILD_SHARED_LIBS=OFF in cached-get-deps.py) so that
# Bazel test binaries don't need LD_LIBRARY_PATH or RPATH to find versioned
# .so files at runtime.
PREFER_SHARED: set[str] = set()

# System library stubs for deps that aren't from getdeps but are
# referenced by BUCK external_deps.  Used by both generate_module_bazel()
# and generate_module_bazel_getdeps().
SYSTEM_LIB_STUBS: dict[str, str] = {
    "libusb": (
        "cc_library(\n"
        '    name = "libusb",\n'
        '    linkopts = ["-lusb-1.0"],\n'
        '    visibility = ["//visibility:public"],\n'
        ")"
    ),
    "libnl": (
        "cc_library(\n"
        '    name = "libnl",\n'
        '    hdrs = glob(["include/libnl3/**"]),\n'
        '    includes = ["include/libnl3"],\n'
        '    linkopts = ["-lnl-3", "-lnl-route-3"],\n'
        '    visibility = ["//visibility:public"],\n'
        ")"
    ),
    "systemd": (
        "cc_library(\n"
        '    name = "systemd",\n'
        '    hdrs = glob(["include/systemd/**"]),\n'
        '    includes = ["include"],\n'
        '    linkopts = ["-lsystemd"],\n'
        '    visibility = ["//visibility:public"],\n'
        ")"
    ),
    "libcurl": (
        "cc_library(\n"
        '    name = "libcurl",\n'
        '    hdrs = glob(["include/curl/**"]),\n'
        '    includes = ["include"],\n'
        '    linkopts = ["-lcurl"],\n'
        '    visibility = ["//visibility:public"],\n'
        ")"
    ),
}


def _make_build_file_content(  # noqa: PLR0911, PLR0912, PLR0915
    dep: PrebuiltDep,
    bazel_name: str,
    inter_deps: list[str] | None = None,
) -> str | None:
    """Generate build_file_content for a pre-built dep.

    inter_deps: Bazel labels for same-workspace cc_library deps (e.g.
    ["@fmt//:fmt"]).  When None, derived from the manifest at call time
    filtered against prebuilt_names (non-getdeps path only).
    """
    inter_deps = inter_deps or []
    load_stmt = 'load("@rules_cc//cc:defs.bzl", "cc_library")'

    # Special case: googletest needs gtest, gmock, gtest_main targets
    if dep.name == "googletest":
        return f'''{load_stmt}
load("@rules_cc//cc:defs.bzl", "cc_import")
cc_library(
    name = "googletest",
    srcs = ["{dep.lib_dir}/libgtest.a", "{dep.lib_dir}/libgmock.a"],
    hdrs = glob(["include/**"], allow_empty = True),
    includes = ["include"],
    visibility = ["//visibility:public"],
    linkstatic = True,
)
cc_library(
    name = "gtest",
    srcs = ["{dep.lib_dir}/libgtest.a"],
    hdrs = glob(["include/gtest/**"]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    linkstatic = True,
)
cc_library(
    name = "gmock",
    srcs = ["{dep.lib_dir}/libgmock.a"],
    hdrs = glob(["include/gmock/**"]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    linkstatic = True,
    deps = [":gtest"],
)
cc_library(
    name = "gtest_main",
    linkopts = ["-Wl,--whole-archive", "{dep.path}/{dep.lib_dir}/libgtest_main.a", "-Wl,--no-whole-archive"],
    visibility = ["//visibility:public"],
    linkstatic = True,
    deps = [":gtest"],
)
cc_library(
    name = "gmock_main",
    srcs = ["{dep.lib_dir}/libgmock_main.a"],
    visibility = ["//visibility:public"],
    linkstatic = True,
    deps = [":gmock"],
)
'''

    # Special case: folly — use sub-archives to match cmake link behavior.
    #
    # folly installs both a fat archive (libfolly.a, ~140MB, all objects) and
    # ~300 small per-component sub-archives (libfolly_*.a).  cmake's generated
    # config links the sub-archives, so the linker only pulls in objects that
    # are actually referenced — keeping debug sections from unreferenced objects
    # out of the final binary.  Using the fat archive instead would inflate
    # binaries by ~200MB of debug sections from folly's unconditional -g flag.
    if dep.name == "folly":
        deps_list = ", ".join(f'"{d}"' for d in inter_deps)
        sys_linkopts = list(SYSTEM_LINKOPTS.get(dep.name, []))
        linkopts_list = ", ".join(f'"{opt}"' for opt in sys_linkopts)
        return f'''{load_stmt}
cc_library(
    name = "{bazel_name}",
    srcs = glob(
        ["{dep.lib_dir}/libfolly*.a"],
        exclude = [
            "{dep.lib_dir}/libfolly.a",
            "{dep.lib_dir}/libfollybenchmark.a",
        ],
    ),
    hdrs = glob(["include/**"], allow_empty = True),
    includes = ["include"],
    deps = [{deps_list}],
    linkopts = [{linkopts_list}],
    visibility = ["//visibility:public"],
    linkstatic = True,
)
cc_library(
    name = "follybenchmark",
    srcs = ["{dep.lib_dir}/libfollybenchmark.a"],
    deps = [":folly"],
    visibility = ["//visibility:public"],
    linkstatic = True,
)
'''

    # Special case: benchmark — libbenchmark_main.a provides main(), which
    # conflicts with test_main when benchmark is a transitive dep (e.g. via
    # fbthrift).  Expose it as a separate target so only benchmark binaries
    # that explicitly want it link against it.
    if dep.name == "benchmark":
        deps_list = ", ".join(f'"{d}"' for d in inter_deps)
        deps_str_inner = f"\n    deps = [{deps_list}]," if inter_deps else ""
        return f"""{load_stmt}
cc_library(
    name = "benchmark",
    srcs = glob(["lib64/libbenchmark.a", "lib/libbenchmark.a"], allow_empty = True),
    hdrs = glob(["include/**"]),
    includes = ["include"],{deps_str_inner}
    visibility = ["//visibility:public"],
    linkstatic = True,
)
cc_library(
    name = "benchmark_main",
    srcs = glob(["lib64/libbenchmark_main.a", "lib/libbenchmark_main.a"], allow_empty = True),
    deps = [":benchmark"],
    visibility = ["//visibility:public"],
    linkstatic = True,
)
"""

    deps_str = ""
    if inter_deps:
        deps_list = ", ".join(f'"{d}"' for d in inter_deps)
        deps_str = f"\n    deps = [{deps_list}],"

    # fbthrift: also expose the thrift compiler binary so thrift genrules
    # can declare a proper Bazel dependency on it (rather than a hardcoded path).
    exports_files_str = ""
    if dep.name == "fbthrift":
        exports_files_str = '\nexports_files(["bin/thrift1"])\n'

    if dep.is_header_only:
        if dep.include_dir == ".":
            hdrs_glob = 'glob(["**/*.h", "**/*.hpp"], allow_empty = True)'
            includes = '["."]'
            extra_deps_str = deps_str
        elif dep.extra_include_dirs or dep.name == "libsai":
            # libsai's saiobject.h includes experimental headers (saiexperimental*.h)
            # that live only in sai_impl/include/, not in libsai/include/.  Depend on
            # @sai_impl//:sai_impl_headers so sai_impl/include/ is on the include path.
            # We also intentionally do NOT expose libsai/experimental/ (if it exists)
            # because those open-source stubs would shadow the BCM-specific versions in
            # sai_impl/include/ for targets with IS_OSS_BRCM_SAI=true.
            hdrs_glob = 'glob(["include/**"], allow_empty = True)'
            includes = '["include"]'
            sai_impl_dep = '"@sai_impl//:sai_impl_headers"'
            if inter_deps:
                deps_list = ", ".join(f'"{d}"' for d in inter_deps)
                extra_deps_str = f"\n    deps = [{deps_list}, {sai_impl_dep}],"
            else:
                extra_deps_str = f"\n    deps = [{sai_impl_dep}],"
        else:
            hdrs_glob = 'glob(["include/**"], allow_empty = True)'
            includes = '["include"]'
            extra_deps_str = deps_str
        return f'''{load_stmt}
cc_library(
    name = "{bazel_name}",
    hdrs = {hdrs_glob},
    includes = {includes},{extra_deps_str}
    visibility = ["//visibility:public"],
)
'''
    # Special case: sai_impl — generate a sai_impl_headers header-only target
    # in addition to the full sai_impl target with libraries.  libsai depends
    # on sai_impl_headers (not the full sai_impl) so that sai_impl/include/ is
    # on the include path before libsai/experimental/, ensuring BCM-specific
    # extension headers take priority over the generic SAI stubs.
    if dep.name == "sai_impl":
        deps_list = ", ".join(f'"{d}"' for d in inter_deps) if inter_deps else ""
        full_deps = f'[":sai_impl_headers"{", " + deps_list if deps_list else ""}]'
        return f"""{load_stmt}
cc_library(
    name = "sai_impl_headers",
    hdrs = glob(["include/**"]),
    includes = ["include"],
    visibility = ["//visibility:public"],
)
cc_library(
    name = "sai_impl",
    srcs = glob(["lib/*.a", "lib64/*.a"], allow_empty = True),
    deps = {full_deps},
    visibility = ["//visibility:public"],
    linkstatic = True,
)
"""

    if dep.has_static_libs and dep.name not in PREFER_SHARED:
        # Glob both lib/ and lib64/ — on CentOS some deps (fmt, glog, etc.)
        # install to lib64/ instead of lib/.  Globbing both is harmless when
        # only one directory exists.
        lib_globs = '"lib/*.a", "lib64/*.a"'
        # Deps that ship both a fat archive and per-component sub-archives:
        # exclude the fat archive so the linker only pulls in referenced objects
        # (matching cmake behavior and avoiding ~200MB of unreferenced debug
        # sections from deps built with -g).  Also exclude compiler-only archives.
        exclude_libs: list[str] = []
        if dep.name == "fbthrift":
            exclude_libs = [
                "lib/libcompiler.a",
                "lib/libcompiler_ast.a",
                "lib/libcompiler_base.a",
                "lib/libcompiler_lib.a",
                "lib/libwhisker.a",
            ]
        elif dep.name == "mvfst":
            exclude_libs = ["lib/libmvfst.a"]
        elif dep.name == "boost":
            exclude_libs = [
                "lib/libboost_prg_exec_monitor.a",
                "lib/libboost_test_exec_monitor.a",
                "lib/libboost_unit_test_framework.a",
                "lib64/libboost_prg_exec_monitor.a",
                "lib64/libboost_test_exec_monitor.a",
                "lib64/libboost_unit_test_framework.a",
            ]
        lib_exclude = ""
        if exclude_libs:
            quoted = ", ".join(f'"{lib}"' for lib in exclude_libs)
            lib_exclude = f", exclude = [{quoted}]"
        includes = f'["{dep.include_dir}"]' if dep.include_dir else "[]"
        hdrs_glob = 'glob(["include/**"])' if dep.include_dir == "include" else "[]"
        sys_linkopts = list(SYSTEM_LINKOPTS.get(dep.name, []))
        linkopts_str = ""
        if sys_linkopts:
            linkopts_list = ", ".join(f'"{opt}"' for opt in sys_linkopts)
            linkopts_str = f"\n    linkopts = [{linkopts_list}],"
        return f'''{load_stmt}
cc_library(
    name = "{bazel_name}",
    srcs = glob([{lib_globs}]{lib_exclude}, allow_empty = True),
    hdrs = {hdrs_glob},
    includes = {includes},{deps_str}{linkopts_str}
    visibility = ["//visibility:public"],
    linkstatic = True,
)
{exports_files_str}'''
    if dep.has_shared_libs:
        so_file = dep.shared_libs[0] if dep.shared_libs else ""
        includes = f'["{dep.include_dir}"]' if dep.include_dir else "[]"
        hdrs_glob = 'glob(["include/**"])' if dep.include_dir == "include" else "[]"
        all_deps = [f'":{bazel_name}_import"'] + (
            [f'"{d}"' for d in inter_deps] if inter_deps else []
        )
        all_deps_str = ", ".join(all_deps)
        hdrs_line = f"\n    hdrs = {hdrs_glob}," if hdrs_glob != "[]" else ""
        includes_line = f"\n    includes = {includes}," if includes != "[]" else ""
        # Bazel's cc_import does not support hdrs= or includes=; put those
        # on the wrapping cc_library instead.
        return f'''{load_stmt}
load("@rules_cc//cc:defs.bzl", "cc_import")
cc_import(
    name = "{bazel_name}_import",
    shared_library = "{dep.lib_dir}/{so_file}",
    visibility = ["//visibility:private"],
)
cc_library(
    name = "{bazel_name}",{hdrs_line}{includes_line}
    deps = [{all_deps_str}],
    linkopts = ["-Wl,-rpath,{dep.path}/{dep.lib_dir}"],
    visibility = ["//visibility:public"],
)
'''
    return None


# ---------------------------------------------------------------------------
# Dependency resolver
# ---------------------------------------------------------------------------


class DepResolver:
    """Resolve Buck dependencies to Bazel labels."""

    def __init__(
        self,
        prebuilt_deps: dict[str, PrebuiltDep],
        all_targets: dict[str, BuckTarget],
        repo_root: str = ".",
        bzl_include_pkgs: set[str] | None = None,
    ):
        self.prebuilt_deps = prebuilt_deps
        self.all_targets = all_targets
        self._repo_root = repo_root
        self.warnings: list[str] = []
        # Set of target labels that are skipped (all-facebook sources)
        # Populated by BuildFileGenerator before resolution
        self.skipped_targets: set[str] = set()
        # Packages that have # bazelify: include annotations — their .bzl
        # macros may define targets that other packages depend on.
        self._bzl_include_pkgs = bzl_include_pkgs or set()
        # Target names that were explicitly excluded via # bazelify: exclude.
        # These may be re-created by .bzl macros, so cross-package refs
        # should be kept.  Populated by BuildFileGenerator.
        self._excluded_targets: set[str] = set()
        # Build set of known FBOSS target full names
        self.known_targets = set(all_targets.keys())
        # Build thrift target name mapping: "name-cpp2-types" -> "name"
        self._thrift_base_names: dict[str, str] = {}
        for full_name, target in all_targets.items():
            if target.rule_type == "thrift_library":
                for suffix in THRIFT_SUFFIXES:
                    self._thrift_base_names[f"{full_name}{suffix}"] = full_name

    def resolve(self, dep: str, from_target: BuckTarget) -> str | None:  # noqa: PLR0911, PLR0912
        """Resolve a single Buck dep to a Bazel label, or None if unresolvable."""
        dep = dep.strip()

        # 1. Relative dep (:name) - stays as-is
        if dep.startswith(":"):
            # Check if it's a thrift sub-target
            base_name = dep.split(":")[1]
            for suffix in THRIFT_SUFFIXES:
                if base_name.endswith(suffix):
                    thrift_name = base_name[: -len(suffix)]
                    return f":{thrift_name}"
            # Verify the target exists in this package
            full_label = f"{from_target.buck_path}:{base_name}"
            if full_label not in self.all_targets:
                return None  # target not parsed (custom macro, etc.)
            return dep

        # 2. fbsource//third-party/* deps
        for prefix, label in FBSOURCE_THIRD_PARTY_MAP.items():
            if dep.startswith(prefix):
                if self._is_available_repo(label):
                    return label
                return None
        if dep.startswith("fbsource//"):
            # Unknown fbsource dep - skip with warning
            self.warnings.append(
                f"  Skipping unknown fbsource dep: {dep} "
                f"(from {from_target.buck_path}:{from_target.name})"
            )
            return None

        # 3. External dep (from EXTERNAL_DEP_MAP)
        for prefix, label in EXTERNAL_DEP_MAP.items():
            if dep.startswith(prefix):
                if self._is_available_repo(label):
                    return label
                return None

        # 4. FBOSS or common/configerator dep
        if (
            dep.startswith("//fboss/")
            or dep.startswith("//common/network/")
            or dep.startswith("//common/fb303/")
            or dep.startswith("//common/stats/")
            or dep.startswith("//configerator/structs/neteng/fboss/thrift")
        ):
            # Skip facebook/ internal paths
            if "/facebook/" in dep or "/facebook:" in dep:
                return None
            # Check for thrift sub-target
            for suffix in THRIFT_SUFFIXES:
                if dep.endswith(suffix):
                    return dep[: -len(suffix)]
            # Verify the target exists in our parsed targets or hand-written BUILD files
            if ":" in dep and dep not in self.all_targets:
                dep_pkg_dir = dep.rsplit(":", 1)[0].lstrip("/")
                # Check hand-written BUILD files that exist independently of the converter
                build_path = Path(self._repo_root) / dep_pkg_dir / "BUILD.bazel"
                buck_path = Path(self._repo_root) / dep_pkg_dir / "BUCK"
                if build_path.exists() and not buck_path.exists():
                    # Hand-written BUILD (no BUCK) - trust it has the target
                    return dep
                # Check if the package has .bzl includes that may define the
                # target at Bazel evaluation time.  Only trust targets that
                # were explicitly excluded (they exist in the BUCK but were
                # annotated with # bazelify: exclude), not targets from
                # internal macros that were never parsed.
                dep_buck_path = "//" + dep_pkg_dir
                dep_label = dep.rsplit(":", 1)[1] if ":" in dep else ""
                if (
                    dep_buck_path in self._bzl_include_pkgs
                    and dep_label in self._excluded_targets
                ):
                    return dep
                # For packages with BUCK files, if the target wasn't parsed it's
                # from a custom macro we skip. Drop it.
                return None
            return dep

        # 5. Meta-internal dep
        if dep.startswith("//"):
            for prefix in INTERNAL_DEP_PREFIXES:
                if dep.startswith(prefix):
                    return None  # strip silently
            # Unknown // dep
            self.warnings.append(
                f"  Skipping unknown dep: {dep} "
                f"(from {from_target.buck_path}:{from_target.name})"
            )
            return None

        return dep

    def _is_available_repo(self, label: str) -> bool:
        """Check if a @repo//... label references an available pre-built dep."""
        if not label.startswith("@"):
            return True  # local dep, always available
        # Extract repo name from @repo//:target
        repo = label.split("//", maxsplit=1)[0].lstrip("@")
        if repo in SYSTEM_LIB_STUBS:
            return True
        # Check both the Bazel name (underscores) and canonical name (dashes)
        canonical = repo.replace("_", "-")
        return repo in self.prebuilt_deps or canonical in self.prebuilt_deps

    def resolve_external_dep(self, dep: str) -> str | None:
        """Resolve an exported_external_deps or external_deps entry."""
        # dep can be "pkg" or "pkg:lib"
        pkg = dep.split(":", 1)[0] if ":" in dep else dep

        if pkg in THIRD_PARTY_MAP:
            label = THIRD_PARTY_MAP[pkg]
            if label is None:
                return None  # explicitly skipped
            if self._is_available_repo(label):
                return label
            self.warnings.append(f"  Skipping unavailable dep: {dep} -> {label}")
            return None

        self.warnings.append(f"  Skipping unknown external dep: {dep}")
        return None

    def resolve_all(self, target: BuckTarget) -> list[str]:  # noqa: PLR0912
        """Resolve all deps for a target, returning Bazel labels."""
        bazel_deps = []
        seen = set()

        # Regular deps -> implementation_deps in Bazel (but we use deps for now)
        for dep in target.deps:
            label = self.resolve(dep, target)
            if label and label not in seen:
                bazel_deps.append(label)
                seen.add(label)

        # Exported deps -> deps in Bazel
        for dep in target.exported_deps:
            label = self.resolve(dep, target)
            if label and label not in seen:
                bazel_deps.append(label)
                seen.add(label)

        # cpp2_deps (for thrift libraries)
        for dep in target.cpp2_deps:
            label = self.resolve(dep, target)
            if label and label not in seen:
                bazel_deps.append(label)
                seen.add(label)

        # External deps
        for dep in target.external_deps + target.exported_external_deps:
            label = self.resolve_external_dep(dep)
            if label and label not in seen:
                bazel_deps.append(label)
                seen.add(label)

        # Extra deps from annotations (# bazelify: extra_dep = //path:target)
        for extra in target.annotations.get("extra_dep", []):
            label = extra.strip()
            if label and label not in seen:
                bazel_deps.append(label)
                seen.add(label)

        # Filter out deps to targets that are skipped (all-facebook sources)
        if self.skipped_targets:
            filtered = []
            for d in bazel_deps:
                if d in self.skipped_targets:
                    continue
                if d.startswith(":"):
                    full = f"{target.buck_path}{d}"
                    if full in self.skipped_targets:
                        continue
                filtered.append(d)
            bazel_deps = filtered

        return sorted(bazel_deps)


# ---------------------------------------------------------------------------
# BUILD file generator
# ---------------------------------------------------------------------------


class BuildFileGenerator:
    """Generate Bazel BUILD files from parsed Buck targets."""

    def __init__(
        self,
        resolver: DepResolver,
        repo_root: str,
        bzl_includes: dict[str, list[tuple[str, str]]] | None = None,
    ):
        self.resolver = resolver
        self.repo_root = Path(repo_root)
        self.generated_count = 0
        self.skipped_targets: list[str] = []
        self._skipped_target_labels: set[str] = set()
        # Bazel-native includes: load() + call a .bzl macro in the BUILD file.
        self._bzl_includes = bzl_includes or {}
        # Set of Buck package paths (e.g. "//fboss/cli/fboss2/commands/show/acl")
        # where all targets are thrift_library.  These packages get an auto-generated
        # _impl cc_library for any .cpp files that live in their directory, so the
        # parent package can depend on that target instead of trying to reference
        # source files across a Bazel package boundary.
        self._thrift_only_pkgs: set[str] = set()
        # Deps to use for the auto-generated _impl targets in thrift-only subpackages.
        # Populated by _translate_cpp_library when a parent target strips files from a
        # thrift-only subpackage.  Maps subpkg path → resolved dep list.
        self._thrift_impl_deps: dict[str, list[str]] = {}
        # Set of Buck package paths that contain at least one C++ target.
        # Pre-computed from all_targets in generate_all() to quickly distinguish
        # "real" C++ packages (= Bazel package boundaries) from BUCK files that
        # only define Python or other non-C++ targets (e.g. fboss/lib/oss/BUCK).
        self._cpp_pkgs: set[str] = set()

    def generate_all(  # noqa: PLR0912
        self,
        targets_by_path: dict[str, list[BuckTarget]],
        exclude_all_pkgs: set[str] | None = None,
    ):
        """Generate BUILD.bazel files for all parsed Buck paths."""
        _exclude_all = exclude_all_pkgs or set()
        # Pre-compute the set of C++ package paths (from all_targets), used by
        # _filter_files to avoid treating Python-only BUCK files as boundaries.
        _cpp_rule_types = {
            "cpp_library",
            "cpp_binary",
            "cpp_unittest",
            "cpp_benchmark",
            "thrift_library",
            "custom_rule",
        }
        for label, tgt in self.resolver.all_targets.items():
            if tgt.rule_type in _cpp_rule_types:
                pkg = label.rsplit(":", 1)[0]
                self._cpp_pkgs.add(pkg)
        # First pass: identify targets that will be skipped (all-facebook sources)
        # and identify thrift-only packages.
        for buck_path, targets in targets_by_path.items():
            for target in targets:
                if self._will_skip_target(target):
                    self._skipped_target_labels.add(f"{buck_path}:{target.name}")
            # A package is thrift-only when every target is a thrift_library AND
            # the package hasn't been annotated with # bazelify: exclude-all.
            # Packages with exclude-all may have .cpp files that must NOT be compiled
            # (e.g. fboss/agent/hw/bcm which requires the proprietary BCM SDK).
            if (
                targets
                and all(t.rule_type == "thrift_library" for t in targets)
                and buck_path not in _exclude_all
            ):
                self._thrift_only_pkgs.add(buck_path)
        # Share skipped targets with the resolver so it can filter deps
        self.resolver.skipped_targets = self._skipped_target_labels
        # Second pass: generate BUILD files (also populates _thrift_impl_deps)
        for buck_path, targets in sorted(targets_by_path.items()):
            self._generate_build_file(buck_path, targets)
        # Also generate BUILD files for packages that only have bzl_includes
        # (all BUCK targets excluded, but hand-written .bzl targets needed).
        for buck_path in sorted(self._bzl_includes):
            if buck_path not in targets_by_path or not targets_by_path[buck_path]:
                self._generate_build_file(buck_path, [])
        # Third pass: re-generate thrift-only subpackage BUILD.bazel files that now
        # have inherited deps recorded by parent packages in _thrift_impl_deps.
        for buck_path in sorted(self._thrift_impl_deps):
            if buck_path in targets_by_path:
                self._generate_build_file(buck_path, targets_by_path[buck_path])
        # Cleanup pass: for packages annotated with # bazelify: exclude-all that
        # produced no targets (e.g. only cpp tests), delete any stale AUTO-GENERATED
        # BUILD.bazel that was left over from before the annotation was added.
        for buck_path in _exclude_all:
            if targets_by_path.get(buck_path):
                # Has thrift targets → was regenerated above, nothing to do.
                continue
            rel_dir = buck_path.lstrip("/")
            stale = self.repo_root / rel_dir / "BUILD.bazel"
            if stale.is_file():
                first_line = stale.read_text().split("\n", 1)[0]
                if "AUTO-GENERATED" in first_line:
                    stale.unlink()
                    print(f"  Deleted stale BUILD.bazel: {rel_dir}/BUILD.bazel")
        # Cleanup pass: find AUTO-GENERATED BUILD.bazel files whose BUCK source
        # no longer exists (e.g. BUCK file deleted).  These are orphaned and
        # would cause build failures referencing non-existent source files.
        # Scan fboss/ and common/ (not build/ — _create_getdeps_build_files()
        # writes its own AUTO-GENERATED markers there independently).
        generated_paths = {
            buck_path.lstrip("/")
            for buck_path in {
                *targets_by_path.keys(),
                *_exclude_all,
                *self._bzl_includes,
            }
        }
        # _cleanup_orphaned_build_files() handles the BUCK-existence check
        # (no BUCK file at all).  Run it here so the full-regen path also
        # catches files in packages where all targets were excluded (those
        # appear in generated_paths but produced no output).  The standalone
        # function is also called by main() on --if-needed skips.
        _cleanup_orphaned_build_files(self.repo_root)
        # Also remove generated BUILD.bazel files whose package still has a
        # BUCK file but produced no output (all targets excluded, etc.).
        for scan_subdir in ("fboss", "common"):
            scan_dir = self.repo_root / scan_subdir
            if not scan_dir.is_dir():
                continue
            find_args = [
                "find", str(scan_dir),
                "(", "-name", ".*", "-o", "-name", "bazel-*", "-o", "-name", "__pycache__", ")", "-prune",
                "-o", "-name", "BUILD.bazel", "-type", "f", "-print",
            ]  # fmt: skip
            result = subprocess.run(
                find_args, check=False, capture_output=True, text=True
            )
            for line in sorted(result.stdout.splitlines()):
                if not line:
                    continue
                build_file = Path(line)
                rel_dir = str(build_file.parent.relative_to(self.repo_root))
                if rel_dir not in generated_paths:
                    first_line = build_file.read_text().split("\n", 1)[0]
                    if "AUTO-GENERATED" in first_line:
                        build_file.unlink()
                        print(f"  Deleted stale BUILD.bazel: {rel_dir}/BUILD.bazel")

    def _will_skip_target(self, target: BuckTarget) -> bool:
        """Check if a target will be skipped due to all-facebook sources."""
        if target.rule_type in ("thrift_library", "export_file"):
            return False
        srcs = target.srcs
        hdrs = target.headers
        if not srcs and not hdrs:
            return False
        # A target with add_src annotations will have OSS sources injected
        # during translation, so it should not be skipped even if all original
        # srcs are facebook/ internal.
        if target.annotations.get("add_src"):
            return False
        filtered_srcs = [
            s for s in srcs if "/facebook/" not in s and not s.startswith("facebook/")
        ]
        filtered_hdrs = [
            h for h in hdrs if "/facebook/" not in h and not h.startswith("facebook/")
        ]
        return not filtered_srcs and not filtered_hdrs and bool(srcs or hdrs)

    def _generate_build_file(self, buck_path: str, targets: list[BuckTarget]):  # noqa: PLR0912, PLR0915
        """Generate a single BUILD.bazel file."""
        rel_dir = buck_path.lstrip("/")
        build_dir = self.repo_root / rel_dir
        build_file = build_dir / "BUILD.bazel"

        if rel_dir in {"", "."}:
            return
        if not build_dir.is_dir():
            return

        rules = []
        needs_thrift_load = False
        needed_rules = set()

        for target in targets:
            rule = self._translate_target(target)
            if rule:
                rules.append(rule)
                if target.rule_type == "thrift_library":
                    needs_thrift_load = True
                elif target.rule_type == "cpp_library":
                    needed_rules.add("cc_library")
                elif target.rule_type in {"cpp_binary", "cpp_benchmark"}:
                    needed_rules.add("cc_binary")
                elif target.rule_type == "cpp_unittest":
                    needed_rules.add("cc_test")

        # For thrift-only packages: auto-generate a _impl cc_library for any
        # .cpp files in the directory, BUT ONLY when a parent package has
        # actually referenced those files (i.e. _thrift_impl_deps was populated
        # by _translate_cpp_library).  This prevents spurious _impl targets for
        # packages like fboss/agent/hw/sai/switch that happen to look thrift-only
        # (only a thrift_library BUCK target survives the parser) but whose .cpp
        # files must NOT be compiled via this mechanism.
        if buck_path in self._thrift_only_pkgs and buck_path in self._thrift_impl_deps:
            # Collect .cpp/.h files in this package directory, including
            # subdirectories that do NOT have their own BUCK files (those are
            # still part of this Bazel package).
            cpp_files: list[str] = []
            hdrs_impl: list[str] = []

            def _pkg_scan(d: Path, prefix: str) -> None:
                for item in sorted(d.iterdir()):
                    rel = prefix + item.name
                    if item.is_dir():
                        if (item / "BUCK").exists():
                            continue  # separate Bazel package, skip
                        _pkg_scan(item, rel + "/")
                    elif item.is_file():
                        if item.name.startswith(".") or "/facebook/" in item.name:
                            continue
                        if item.suffix == ".cpp":
                            cpp_files.append(rel)
                        elif item.suffix == ".h":
                            hdrs_impl.append(rel)

            _pkg_scan(build_dir, "")
            cpp_files.sort()
            hdrs_impl.sort()
            if cpp_files:
                # Deps for the _impl target:
                # - ":model" (or whichever thrift target) for generated headers
                # - Inherited deps from the parent that strips these .cpp files,
                #   so that transitive headers (like CLI11) are available.
                thrift_deps = [
                    f":{t.name}" for t in targets if t.rule_type == "thrift_library"
                ]
                inherited = self._thrift_impl_deps.get(buck_path, [])
                impl_deps = list(thrift_deps)
                # Absolute form of this package's own targets (e.g. //pkg:model)
                self_pkg_prefix = f"{buck_path}:"
                for d in inherited:
                    if d.endswith(":_impl"):
                        continue  # don't inherit _impl cross-refs
                    # Deduplicate: skip absolute labels that are already
                    # present as relative labels (e.g. //this/pkg:model vs :model)
                    if d.startswith(self_pkg_prefix):
                        rel = ":" + d[len(self_pkg_prefix) :]
                        if rel in impl_deps:
                            continue
                    if d not in impl_deps:
                        impl_deps.append(d)
                impl_lines = ["cc_library("]
                impl_lines.append('    name = "_impl",')
                impl_lines.append(f"    srcs = {self._format_string_list(cpp_files)},")
                if hdrs_impl:
                    impl_lines.append(
                        f"    hdrs = {self._format_string_list(hdrs_impl)},"
                    )
                if impl_deps:
                    impl_lines.append(
                        f"    deps = {self._format_string_list(impl_deps)},"
                    )
                impl_lines.append('    visibility = ["//visibility:public"],')
                impl_lines.append(")")
                impl_lines.append("")
                rules.append("\n".join(impl_lines))
                needed_rules.add("cc_library")

        bzl_includes = self._bzl_includes.get(buck_path, [])
        if not rules and not bzl_includes:
            return

        # Write BUILD.bazel
        lines = [
            "# AUTO-GENERATED by bazelify.py - DO NOT EDIT",
            f"# Source: {rel_dir}/BUCK",
            "",
        ]

        # Add load() statements for native rules (required in Bazel 9+)
        cc_rules = sorted(needed_rules & {"cc_library", "cc_binary", "cc_test"})
        if cc_rules:
            rule_list = ", ".join(f'"{r}"' for r in cc_rules)
            lines.append(f'load("@rules_cc//cc:defs.bzl", {rule_list})')

        if needs_thrift_load:
            lines.append(
                'load("//fboss/build_defs:thrift_library.bzl", "fboss_thrift_library")'
            )

        # Bazel-native includes: load and call .bzl macros that define
        # hand-written targets (e.g. SAI switch libraries not auto-generated
        # from BUCK).  Annotation: # bazelify: include <file.bzl>:<func>
        for bzl_file, func_name in bzl_includes:
            lines.append(f'load(":{bzl_file}", "{func_name}")')

        if cc_rules or needs_thrift_load or bzl_includes:
            lines.append("")

        lines.extend(rules)

        for _, func_name in bzl_includes:
            lines.append(f"{func_name}()")
            lines.append("")

        build_file.write_text("\n".join(lines) + "\n")
        self.generated_count += 1

    def _translate_target(self, target: BuckTarget) -> str | None:  # noqa: PLR0911
        """Translate a Buck target to a Bazel rule string."""
        if target.rule_type == "cpp_library":
            return self._translate_cpp_library(target)
        if target.rule_type == "cpp_binary":
            return self._translate_cpp_binary(target)
        if target.rule_type == "cpp_unittest":
            return self._translate_cpp_test(target)
        if target.rule_type == "cpp_benchmark":
            return self._translate_cpp_benchmark(target)
        if target.rule_type == "thrift_library":
            return self._translate_thrift_library(target)
        if target.rule_type == "export_file":
            return self._translate_export_file(target)
        if target.rule_type == "custom_rule":
            return self._translate_custom_rule(target)
        if target.rule_type == "buck_filegroup":
            return self._translate_filegroup(target)
        # Skip python targets for now
        return None

    def _filter_files(self, files: list[str], buck_path: str) -> list[str]:  # noqa: PLR0912
        """Filter out facebook/ internal files and files that cross into subpackages.
        Also substitutes oss/ replacement files and deduplicates.
        Handles Buck genrule output references like ':target[Output.h]'."""
        seen = set()
        result = []
        rel_dir = buck_path.lstrip("/")
        build_dir = self.repo_root / rel_dir

        for f in files:
            is_genrule_src = False
            # Handle Buck genrule output references: ":target[Output.h]" -> ":target"
            if f.startswith(":") and "[" in f:
                f = f.split("[")[0]  # noqa: PLW2901
                is_genrule_src = True
            # Handle fully-qualified Buck genrule output refs in the same package:
            # "//pkg:target[Output.cpp]" -> ":target" when pkg == buck_path
            elif f.startswith("//") and "[" in f:
                label_part = f.split("[")[0]  # "//pkg:target"
                if ":" in label_part:
                    pkg_part, tgt_part = label_part.rsplit(":", 1)
                    if pkg_part == buck_path:
                        f = f":{tgt_part}"  # noqa: PLW2901
                        is_genrule_src = True
            if f in seen:
                continue
            seen.add(f)
            # Genrule output label references (e.g. ":gen_config") are valid srcs
            # even though they don't end with a source file extension.
            if is_genrule_src:
                result.append(f)
                continue
            # Skip BUILD labels (//pkg:tgt, :tgt, @repo//...) — these are
            # library references that belong in deps, not file paths.
            if (
                f.startswith("//")
                or f.startswith("@")
                or (f.startswith(":") and not f.endswith((".h", ".cpp", ".c", ".cc")))
            ):
                continue
            # Skip facebook/ internal files (at any depth in the path)
            if "/facebook/" in f or f.startswith("facebook/"):
                continue
            # Skip files that cross into a subdirectory that is a real C++ Bazel
            # package (BUCK file + at least one C++ target).  We intentionally
            # exclude Python-only BUCK directories (e.g. fboss/lib/oss/BUCK which
            # has only a python_binary) from being treated as package boundaries.
            # Thrift-only subpackages get auto-generated _impl cc_library targets
            # (see _get_thrift_subpkg_extra_deps), so we still skip the source
            # file here — the parent depends on the subpackage's _impl target.
            if "/" in f:
                parts = f.split("/")
                crosses_subpackage = False
                for i in range(1, len(parts)):
                    subdir = "/".join(parts[:i])
                    subdir_path = build_dir / subdir
                    if (subdir_path / "BUCK").exists():
                        subpkg = f"//{rel_dir}/{subdir}"
                        if subpkg in self._cpp_pkgs or subpkg in self._thrift_only_pkgs:
                            crosses_subpackage = True
                            break
                    # Hand-written BUILD.bazel files (no BUCK) also define
                    # Bazel subpackages that we cannot include files from.
                    elif (subdir_path / "BUILD.bazel").exists():
                        crosses_subpackage = True
                        break
                if crosses_subpackage:
                    continue

            result.append(f)
        return result

    def _get_thrift_subpkg_extra_deps(
        self, files: list[str], buck_path: str
    ) -> list[str]:
        """Return dep labels for thrift-only subpackages crossed by *files*.

        When a parent target lists source files that live in child directories
        that are thrift-only Bazel packages (only thrift_library targets), those
        files are stripped by _filter_files().  This method returns the
        ``<subpkg>:_impl`` cc_library labels that must be added to the parent
        target so the symbols defined in those .cpp files are still linked.

        We walk all intermediate directories from shallowest to deepest and
        track the *deepest* thrift-only package boundary crossed.  This handles
        nested thrift-only packages like:
            commands/show/fabric/         (thrift-only)
            commands/show/fabric/inputbalance/  (also thrift-only, deeper)
        A file in the deeper package should yield the deeper _impl dep, not the
        shallower one.  If a non-thrift-only BUCK boundary is encountered at any
        depth, we stop looking (the file belongs to that non-thrift package).
        """
        extra: list[str] = []
        seen_pkgs: set[str] = set()
        rel_dir = buck_path.lstrip("/")
        build_dir = self.repo_root / rel_dir

        for f in files:
            if "/facebook/" in f or f.startswith("facebook/"):
                continue
            if "/" not in f:
                continue
            parts = f.split("/")
            deepest_thrift_subpkg: str | None = None
            for i in range(1, len(parts)):
                subdir = "/".join(parts[:i])
                subdir_path = build_dir / subdir
                if (subdir_path / "BUCK").exists():
                    subpkg = f"//{rel_dir}/{subdir}"
                    if subpkg in self._thrift_only_pkgs:
                        # Keep walking deeper to find the innermost boundary
                        deepest_thrift_subpkg = subpkg
                    else:
                        # Hit a non-thrift BUCK boundary — file belongs there,
                        # no _impl dep needed from us
                        deepest_thrift_subpkg = None
                        break
            if deepest_thrift_subpkg and deepest_thrift_subpkg not in seen_pkgs:
                seen_pkgs.add(deepest_thrift_subpkg)
                extra.append(f"{deepest_thrift_subpkg}:_impl")
        return extra

    def _format_string_list(self, items: list[str], indent: int = 8) -> str:
        """Format a list of strings for BUILD file output."""
        if not items:
            return "[]"
        if len(items) == 1:
            return f'["{items[0]}"]'
        pad = " " * indent
        lines = ["["]
        for item in items:
            lines.append(f'{pad}"{item}",')
        lines.append(f"{' ' * (indent - 4)}]")
        return "\n".join(lines)

    def _get_copts(self, target: BuckTarget) -> list[str]:
        """Get compiler options for a target."""
        copts = list(target.compiler_flags)
        for flag in target.preprocessor_flags + target.propagated_pp_flags:
            if flag not in copts:
                copts.append(flag)
        return copts

    def _get_defines(self, target: BuckTarget) -> list[str]:
        """Extract -D defines from propagated_pp_flags."""
        defines = []
        for flag in target.propagated_pp_flags:
            if flag.startswith("-D"):
                defines.append(flag[2:])
        return defines

    def _glob_local_headers(self, target: BuckTarget) -> list[str]:
        """Glob for .h files in the target's directory that aren't declared elsewhere."""
        rel_dir = target.buck_path.lstrip("/")
        pkg_dir = self.repo_root / rel_dir
        if not pkg_dir.is_dir():
            return []
        # Find all .h files directly in the package dir (not in subdirs with BUCK)
        headers = []
        for f in sorted(pkg_dir.iterdir()):
            if f.suffix in (".h", ".tcc") and f.is_file():
                fname = f.name
                if not fname.startswith("facebook"):
                    headers.append(fname)
        return headers

    def _translate_cpp_library(self, target: BuckTarget) -> str | None:  # noqa: PLR0912, PLR0915
        """Translate cpp_library to cc_library."""
        srcs = self._filter_files(target.srcs, target.buck_path)
        hdrs = self._filter_files(target.headers, target.buck_path)
        # Move any BUILD labels from srcs to extra_deps
        # (BUCK allows library labels in srcs; Bazel requires them in deps)
        _src_labels = [
            f
            for f in target.srcs
            if f.startswith("//")
            or f.startswith("@")
            or (f.startswith(":") and not f.endswith((".h", ".cpp", ".c", ".cc")))
        ]

        rel_dir = target.buck_path.lstrip("/")
        pkg_dir = self.repo_root / rel_dir

        # Add extra sources from annotations (# bazelify: add_src = oss/Foo.cpp)
        for add_src in target.annotations.get("add_src", []):
            src_path = add_src.strip()
            if src_path and src_path not in srcs:
                # Skip if any ancestor directory of the file (up to the package
                # root) contains a BUILD.bazel — any such directory is a Bazel
                # subpackage and its files cannot be referenced from the parent.
                if "/" in src_path:
                    parts = src_path.split("/")
                    in_subpkg = False
                    for i in range(1, len(parts)):
                        ancestor = "/".join(parts[:i])
                        if (pkg_dir / ancestor / "BUILD.bazel").exists():
                            in_subpkg = True
                            break
                    if in_subpkg:
                        continue
                srcs.append(src_path)

        # FBOSS BUCK files rarely declare all headers - they rely on Buck's
        # auto-header behavior. For Bazel, we need all .h files in hdrs to
        # avoid "undeclared inclusion" errors. Discover headers from:
        # 1. All .h files in dirs that contain source files listed in srcs
        # 2. All .h files in the package root directory
        hdrs_set = set(hdrs)
        # Collect all subdirectories that contain source files
        src_dirs = {"."}
        for s in srcs:
            if "/" in s:
                src_dirs.add(str(Path(s).parent))
        for src_dir in sorted(src_dirs):
            scan_dir = pkg_dir / src_dir if src_dir != "." else pkg_dir
            if not scan_dir.is_dir():
                continue
            for hdr_path in sorted(scan_dir.glob("*.h")):
                h = str(hdr_path.relative_to(pkg_dir))
                if (
                    h not in hdrs_set
                    and "/facebook/" not in h
                    and not h.startswith("facebook/")
                ):
                    hdrs.append(h)
                    hdrs_set.add(h)

        # If all sources were facebook/ internal, check if it's header-only
        if not srcs and not hdrs and (target.srcs or target.headers):
            self.skipped_targets.append(
                f"{target.buck_path}:{target.name} (all sources are facebook/ internal)"
            )
            return None

        deps = self.resolver.resolve_all(target)
        # Add BUILD labels that were originally in srcs (BUCK allows this, Bazel doesn't)
        for lbl in _src_labels:
            resolved = self.resolver.resolve(lbl, target)
            if resolved and resolved not in deps:
                deps.append(resolved)

        # Auto-add deps for thrift-only subpackages whose .cpp files were stripped.
        # Also record this target's full dep list so the subpackage _impl targets
        # can inherit it (they need the same headers to compile their .cpp files).
        extra_subpkg_deps = self._get_thrift_subpkg_extra_deps(
            target.srcs, target.buck_path
        )
        for extra in extra_subpkg_deps:
            if extra not in deps:
                deps.append(extra)
        if extra_subpkg_deps:
            # Build the inherited dep list for _impl targets: take the parent's
            # resolved deps, convert relative labels to absolute (since they will
            # be used in a different package), and strip _impl labels.
            parent_pkg = target.buck_path  # e.g. "//fboss/cli/fboss2"
            inherited = []
            for d in deps:
                if d.endswith(":_impl"):
                    continue  # skip _impl cross-refs
                if d.startswith(":"):
                    # Relative dep — make absolute using parent package path
                    d = f"{parent_pkg}{d}"  # noqa: PLW2901
                inherited.append(d)
            for subpkg_label in extra_subpkg_deps:
                subpkg_path = subpkg_label.rsplit(":", 1)[0]  # strip :_impl
                if subpkg_path not in self._thrift_impl_deps:
                    self._thrift_impl_deps[subpkg_path] = inherited

        copts = self._get_copts(target)
        defines = self._get_defines(target)

        lines = ["cc_library("]
        lines.append(f'    name = "{target.name}",')

        if srcs:
            lines.append(f"    srcs = {self._format_string_list(srcs)},")
        if hdrs:
            lines.append(f"    hdrs = {self._format_string_list(hdrs)},")
        if deps:
            lines.append(f"    deps = {self._format_string_list(deps)},")
        if copts:
            lines.append(f"    copts = {self._format_string_list(copts)},")
        if defines:
            lines.append(f"    defines = {self._format_string_list(defines)},")
        if target.link_whole:
            lines.append("    alwayslink = True,")

        lines.append('    visibility = ["//visibility:public"],')
        lines.append(")")
        lines.append("")
        return "\n".join(lines)

    def _translate_cpp_binary(self, target: BuckTarget) -> str | None:
        """Translate cpp_binary to cc_binary."""
        srcs = self._filter_files(target.srcs, target.buck_path)
        if not srcs and target.srcs:
            self.skipped_targets.append(
                f"{target.buck_path}:{target.name} (all sources are facebook/ internal)"
            )
            return None

        deps = self.resolver.resolve_all(target)
        copts = self._get_copts(target)

        lines = ["cc_binary("]
        lines.append(f'    name = "{target.name}",')
        if srcs:
            lines.append(f"    srcs = {self._format_string_list(srcs)},")
        if deps:
            lines.append(f"    deps = {self._format_string_list(deps)},")
        if copts:
            lines.append(f"    copts = {self._format_string_list(copts)},")
        lines.append('    visibility = ["//visibility:public"],')
        lines.append(")")
        lines.append("")
        return "\n".join(lines)

    def _translate_cpp_benchmark(self, target: BuckTarget) -> str | None:
        """Translate cpp_benchmark to cc_binary with follybenchmark dep."""
        srcs = self._filter_files(target.srcs, target.buck_path)
        if not srcs and target.srcs:
            self.skipped_targets.append(
                f"{target.buck_path}:{target.name} (all sources are facebook/ internal)"
            )
            return None

        deps = self.resolver.resolve_all(target)
        # Benchmarks need folly's benchmark library for runBenchmarks() etc.
        bench_dep = "@folly//:follybenchmark"
        if bench_dep not in deps:
            deps.append(bench_dep)
        copts = self._get_copts(target)

        lines = ["cc_binary("]
        lines.append(f'    name = "{target.name}",')
        if srcs:
            lines.append(f"    srcs = {self._format_string_list(srcs)},")
        if deps:
            lines.append(f"    deps = {self._format_string_list(deps)},")
        if copts:
            lines.append(f"    copts = {self._format_string_list(copts)},")
        lines.append('    visibility = ["//visibility:public"],')
        lines.append(")")
        lines.append("")
        return "\n".join(lines)

    def _translate_cpp_test(self, target: BuckTarget) -> str | None:  # noqa: PLR0912
        """Translate cpp_unittest to cc_test."""
        # Collect BUILD labels from srcs before filtering (BUCK allows lib labels in
        # srcs; Bazel requires them in deps).
        _src_labels_test = [
            f
            for f in target.srcs
            if f.startswith("//")
            or f.startswith("@")
            or (f.startswith(":") and not f.endswith((".h", ".cpp", ".c", ".cc")))
        ]
        srcs = self._filter_files(target.srcs, target.buck_path)
        if not srcs and target.srcs:
            self.skipped_targets.append(
                f"{target.buck_path}:{target.name} (all sources are facebook/ internal)"
            )
            return None

        # cc_test doesn't have hdrs - add .h files to srcs so they're visible
        rel_dir = target.buck_path.lstrip("/")
        pkg_dir = self.repo_root / rel_dir
        srcs_set = set(srcs)
        src_dirs = {"."}
        for s in srcs:
            if "/" in s:
                src_dirs.add(str(Path(s).parent))
        for src_dir in sorted(src_dirs):
            scan_dir = pkg_dir / src_dir if src_dir != "." else pkg_dir
            if not scan_dir.is_dir():
                continue
            for hdr_path in sorted(scan_dir.glob("*.h")):
                h = str(hdr_path.relative_to(pkg_dir))
                if h not in srcs_set and "/facebook/" not in h:
                    srcs.append(h)
                    srcs_set.add(h)

        deps = self.resolver.resolve_all(target)
        # Move BUILD labels that were originally in srcs into deps
        for lbl in _src_labels_test:
            resolved = self.resolver.resolve(lbl, target)
            if resolved and resolved not in deps:
                deps.append(resolved)

        # Add the OSS test main (provides main() with folly::Init + gtest)
        # and ensure googletest is in deps for the test framework.
        test_main = "//fboss/util/oss:test_main"
        if test_main not in deps:
            deps.append(test_main)
        if not any("@googletest" in d for d in deps):
            deps.append("@googletest//:googletest")
            deps.sort()

        copts = self._get_copts(target)

        # If test srcs are in subdirectories (e.g., tests/Foo.cpp), they may
        # use bare #include "Header.h" expecting the package root on the
        # include path. Add -I<package_dir> so headers are findable.
        if any("/" in s for s in srcs if s.endswith(".cpp")):
            rel_dir = target.buck_path.lstrip("/")
            copts.append(f"-I{rel_dir}")

        lines = ["cc_test("]
        lines.append(f'    name = "{target.name}",')
        if srcs:
            lines.append(f"    srcs = {self._format_string_list(srcs)},")
        if deps:
            lines.append(f"    deps = {self._format_string_list(deps)},")
        if copts:
            lines.append(f"    copts = {self._format_string_list(copts)},")
        if target.resources:
            lines.append(f"    data = {self._format_string_list(target.resources)},")
        if target.size:
            lines.append(f'    size = "{target.size}",')
        lines.append('    visibility = ["//visibility:public"],')
        lines.append(")")
        lines.append("")
        return "\n".join(lines)

    def _translate_thrift_library(self, target: BuckTarget) -> str | None:
        """Translate thrift_library to fboss_thrift_library."""
        if not target.thrift_srcs:
            return None

        # Only generate C++ (cpp2) - skip if cpp2 not in languages
        if target.languages and "cpp2" not in target.languages:
            return None

        deps = self.resolver.resolve_all(target)

        # Format thrift_srcs as a dict mapping files to service lists
        has_services = any(svcs for svcs in target.thrift_srcs.values())
        if has_services:
            # Use dict form: {"file.thrift": ["Svc1"], "other.thrift": []}
            thrift_dict_lines = ["{"]
            for tf, svcs in target.thrift_srcs.items():
                svcs_str = ", ".join(f'"{s}"' for s in svcs)
                thrift_dict_lines.append(f'        "{tf}": [{svcs_str}],')
            thrift_dict_lines.append("    }")
            thrift_srcs_str = "\n".join(thrift_dict_lines)
        else:
            # Simple list form (no services)
            thrift_files = list(target.thrift_srcs.keys())
            thrift_srcs_str = self._format_string_list(thrift_files)

        lines = ["fboss_thrift_library("]
        lines.append(f'    name = "{target.name}",')
        lines.append(f"    thrift_srcs = {thrift_srcs_str},")
        if target.thrift_cpp2_options:
            lines.append(f'    thrift_cpp2_options = "{target.thrift_cpp2_options}",')
        if deps:
            lines.append(f"    deps = {self._format_string_list(deps)},")
        lines.append('    visibility = ["//visibility:public"],')
        lines.append(")")
        lines.append("")
        return "\n".join(lines)

    def _translate_export_file(self, target: BuckTarget) -> str | None:
        """Translate export_file to exports_files."""
        # Only export the file if it actually exists (export_file targets with
        # src = "facebook/..." are facebook-internal and have no OSS counterpart).
        rel_dir = target.buck_path.lstrip("/")
        file_path = self.repo_root / rel_dir / target.name
        if not file_path.exists():
            return None
        lines = ["exports_files("]
        lines.append(f'    ["{target.name}"],')
        lines.append('    visibility = ["//visibility:public"],')
        lines.append(")")
        lines.append("")
        return "\n".join(lines)

    def _translate_filegroup(self, target: BuckTarget) -> str | None:
        """Translate buck_filegroup to filegroup with glob."""
        lines = ["filegroup("]
        lines.append(f'    name = "{target.name}",')
        if target.srcs:
            lines.append(f"    srcs = {self._format_string_list(target.srcs)},")
        else:
            # Srcs used glob() which wasn't expanded by our parser.
            lines.append('    srcs = glob(["**"], allow_empty = True),')
        lines.append('    visibility = ["//visibility:public"],')
        lines.append(")")
        lines.append("")
        return "\n".join(lines)

    def _translate_custom_rule(self, target: BuckTarget) -> str | None:
        """Translate custom_rule to genrule + cc_library."""
        if not target.build_script_dep or not target.output_gen_files:
            return None

        # Resolve the build script dep
        script_dep = target.build_script_dep
        if script_dep.startswith(":"):
            script_dep = f"//{target.buck_path.lstrip('/')}{script_dep}"

        # Rewrite build_args: replace $(location fbcode//...) with $(location //...)
        build_args = target.build_args.replace("fbcode//", "//")

        # Collect source deps from build_args $(location ...) references
        srcs = []
        for m in re.finditer(r"\$\(location ([^)]+)\)", build_args):
            srcs.append(m.group(1))

        # Rewrite $(location target)/subdir references for filegroups.
        # In Bazel, filegroups expand to multiple files so $(location) fails.
        # Replace with the source directory path directly.
        cmd_args = build_args
        for m in re.finditer(r"\$\(location ([^)]+)\)/(\w+)", cmd_args):
            target_ref = m.group(1)
            subdir = m.group(2)
            # Convert target to source path: //fboss/platform:configs -> fboss/platform/configs
            pkg = target_ref.split(":")[0].lstrip("/")
            cmd_args = cmd_args.replace(m.group(0), f"{pkg}/{subdir}")

        lines = ["genrule("]
        lines.append(f'    name = "{target.name}",')
        if srcs:
            srcs_str = self._format_string_list(srcs)
            lines.append(f"    srcs = {srcs_str},")
        lines.append(f"    outs = {self._format_string_list(target.output_gen_files)},")
        lines.append(f'    tools = ["{script_dep}"],')
        # Set LD_LIBRARY_PATH so the tool binary can find shared libs
        # from getdeps (glog, gflags, fmt, etc. are shared libraries).
        ld_path = (
            "LD_LIBRARY_PATH=$$(echo"
            " /var/FBOSS/tmp_bld_dir/installed/*/lib64"
            " /var/FBOSS/tmp_bld_dir/installed/*/lib"
            " | tr ' ' ':')"
        )
        lines.append(
            f'    cmd = "{ld_path} $(location {script_dep}) {cmd_args}'
            f' --install_dir=$(RULEDIR)",'
        )
        lines.append('    visibility = ["//visibility:public"],')
        lines.append(")")
        lines.append("")
        return "\n".join(lines)


def _detect_folly_f14_mode(folly_dep: PrebuiltDep) -> int | None:
    """Detect folly's F14IntrinsicsMode from the installed libfolly.a.

    Returns the mode enum value (0=None, 1=Simd, 2=SimdAndCrc) or None
    if detection fails (e.g. libfolly.a doesn't exist yet).
    """
    libfolly = Path(folly_dep.path) / folly_dep.lib_dir / "libfolly.a"
    if not libfolly.exists():
        return None
    try:
        result = subprocess.run(
            ["nm", str(libfolly)],
            capture_output=True,
            text=True,
            check=False,
        )
        # Look for the defined (T) F14LinkCheck symbol.
        # The mangled name encodes the mode as E0, E1, or E2.
        for line in result.stdout.splitlines():
            if " T " in line and "F14LinkCheck" in line:
                m = re.search(r"F14IntrinsicsModeE(\d)E", line)
                if m:
                    return int(m.group(1))
    except FileNotFoundError:
        pass
    return None


def _detect_cpu_sse42() -> bool:
    """Check if the build machine's CPU supports SSE 4.2.

    Reads /proc/cpuinfo flags on Linux.  Returns False on other platforms
    or if detection fails.
    """
    try:
        cpuinfo = Path("/proc/cpuinfo").read_text()
        for line in cpuinfo.splitlines():
            if line.startswith("flags"):
                return "sse4_2" in line.split()
    except OSError:
        pass
    return False


def _write_bazelrc_folly(repo_root: Path, folly_dep: PrebuiltDep | None = None) -> None:
    """Write folly F14 flags as a .bazel.d/ fragment.

    folly's F14 containers use a compile-time ABI link-check
    (F14IntrinsicsMode) that requires consumers to be compiled with the
    same SIMD mode as libfolly.a.

    When a pre-built folly exists, we inspect libfolly.a to determine
    the exact mode.  On a fresh build (deps built lazily by Bazel repo
    rules), libfolly.a doesn't exist yet, so we fall back to detecting
    CPU features directly — since both folly and FBOSS are built on the
    same machine, this guarantees a consistent build.
    """
    # Primary: inspect the built library if available.
    mode = _detect_folly_f14_mode(folly_dep) if folly_dep else None

    if mode is not None:
        source = "libfolly.a"
    else:
        # Fallback: detect from CPU features.
        has_sse42 = _detect_cpu_sse42()
        mode = 2 if has_sse42 else 1
        source = "CPU features"

    bazel_d = repo_root / ".bazel.d"
    bazel_d.mkdir(exist_ok=True)
    rc_file = bazel_d / "folly.bazelrc"

    # Mode 2 (SimdAndCrc) requires SSE 4.2.  -msse4.2 is the minimal
    # flag that defines __SSE4_2__, which folly uses to select mode 2.
    # Mode 1 (Simd) only needs SSE2 (baseline x86-64, no extra flag).
    # Mode 0 (None) means no SIMD at all.
    lines = [
        "# Auto-generated by bazelify.py -- do not edit",
        "# Sets compiler flags to match folly's F14IntrinsicsMode so that",
        "# FBOSS code references the same F14LinkCheck<N> symbol as libfolly.a.",
    ]
    if mode == 2:
        lines.append("build --cxxopt=-msse4.2")
        lines.append("build --host_cxxopt=-msse4.2")
        print(f"  F14IntrinsicsMode=2 (SimdAndCrc) → -msse4.2 (from {source})")
    elif mode == 1:
        lines.append("# F14IntrinsicsMode=1 (Simd): SSE2 only, no extra flag needed")
        print(f"  F14IntrinsicsMode=1 (Simd) → no extra flag (from {source})")
    else:
        lines.append("# F14IntrinsicsMode=0 (None): no SIMD")
        print(f"  F14IntrinsicsMode=0 (None) → no extra flag (from {source})")

    rc_file.write_text("\n".join(lines) + "\n")
    print(f"  Wrote {rc_file}")


# ---------------------------------------------------------------------------
# Orphan BUILD.bazel cleanup (also used by --if-needed fast path)
# ---------------------------------------------------------------------------


def _write_refresh_compile_commands_build(repo_root: Path) -> None:
    """Write fboss/oss/refresh/BUILD.bazel for the refresh_compile_commands tool.

    Kept in its own package — and out of fboss/build_defs/ — so the legacy
    OSS Bazel 8 build (Monobuild) never sees the @hedron_compile_commands
    load that only resolves under our Bazel-9 MODULE.bazel.

    Uses "# GENERATED" (not "AUTO-GENERATED") so the orphan-cleanup pass,
    which deletes AUTO-GENERATED files in directories without a BUCK file,
    leaves this file alone.  Combined with the content-equality check below,
    this makes repeated bazelify.py runs no-ops (no mtime churn).
    """
    refresh_dir = repo_root / "fboss" / "oss" / "refresh"
    refresh_dir.mkdir(parents=True, exist_ok=True)
    build_file = refresh_dir / "BUILD.bazel"
    content = (
        "# GENERATED by bazelify.py - DO NOT EDIT\n"
        "\n"
        'load("@hedron_compile_commands//:refresh_compile_commands.bzl",'
        ' "refresh_compile_commands")\n'
        "\n"
        "# Generates compile_commands.json for clang tooling (clang-tidy,\n"
        "# clang-include-cleaner, clangd).  Output goes to\n"
        "# /var/FBOSS/tmp_bld_dir/build/fboss/compile_commands.json\n"
        "# (same location as the cmake build).\n"
        "# Run: bazel run //fboss/oss/refresh:refresh_compile_commands\n"
        "refresh_compile_commands(\n"
        '    name = "refresh_compile_commands",\n'
        '    targets = {"//fboss/...": ""},\n'
        ")\n"
    )
    if build_file.exists() and build_file.read_text() == content:
        return
    build_file.write_text(content)


def _cleanup_orphaned_build_files(repo_root: Path) -> None:
    """Delete AUTO-GENERATED BUILD.bazel files with no corresponding BUCK file.

    Called both from BuildFileGenerator.generate_all() (full run) and from
    main() when --if-needed skips regeneration, so stale files from a previous
    broken run are always removed.
    """
    for scan_subdir in ("fboss", "common"):
        scan_dir = repo_root / scan_subdir
        if not scan_dir.is_dir():
            continue
        find_args = [
            "find", str(scan_dir),
            "(", "-name", ".*", "-o", "-name", "bazel-*", "-o", "-name", "__pycache__", ")", "-prune",
            "-o", "-name", "BUILD.bazel", "-type", "f", "-print",
        ]  # fmt: skip
        result = subprocess.run(find_args, check=False, capture_output=True, text=True)
        for line in sorted(result.stdout.splitlines()):
            if not line:
                continue
            build_file = Path(line)
            buck_file = build_file.parent / "BUCK"
            if not buck_file.exists():
                first_line = build_file.read_text().split("\n", 1)[0]
                if "AUTO-GENERATED" in first_line:
                    rel = build_file.parent.relative_to(repo_root)
                    build_file.unlink()
                    print(f"  Deleted orphaned BUILD.bazel: {rel}/BUILD.bazel")


# ---------------------------------------------------------------------------
# Content-hash staleness check for --if-needed
# ---------------------------------------------------------------------------

_INPUT_CHECKSUM_PREFIX = "# bazelify-input-checksum: "


def _compute_input_checksum(repo_root: Path) -> str:
    """SHA-256 over the contents of every file that can affect generated output.

    Content-based, so immune to timestamp changes from branch switches,
    stash/pop, or rebases.
    """
    hasher = hashlib.sha256()

    file_predicates = [
        "-name", "BUCK",
        "-o", "-name", "*.bzl",
        "-o",
        # Exclude BUILD.bazel: _create_getdeps_build_files() writes
        # manifests/BUILD.bazel *after* this checksum is stored in
        # MODULE.bazel, so including it causes a permanent mismatch
        # (stored checksum lacks the file; next run's includes it).
        "(", "-path", "*/fbcode_builder/manifests/*", "!", "-name", "BUILD.bazel", ")",
        "-o", "-path", "*/fbcode_builder/patches/*",
        "-o", "-name", "bazelify.py", "-path", "*/oss/scripts/*",
    ]  # fmt: skip
    prefix = str(repo_root) + "/"
    found_files = [
        str(p)[len(prefix) :] if str(p).startswith(prefix) else str(p)
        for p in _find_repo_files(repo_root, file_predicates)
    ]

    for rel_path in found_files:
        path = repo_root / rel_path
        if path.is_file():
            hasher.update(rel_path.encode())
            hasher.update(path.read_bytes())

    return hasher.hexdigest()


def _read_stored_checksum(repo_root: Path) -> str | None:
    """Read the input checksum from the first lines of MODULE.bazel."""
    module = repo_root / "MODULE.bazel"
    if not module.exists():
        return None
    # Checksum is on the second line (first is the auto-generated comment)
    for line in module.read_text().splitlines()[:5]:
        if line.startswith(_INPUT_CHECKSUM_PREFIX):
            return line[len(_INPUT_CHECKSUM_PREFIX) :].strip()
    return None


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert BUCK files to Bazel BUILD files for FBOSS",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                              # Convert all BUCK files
  %(prog)s --if-needed                  # Skip if inputs unchanged (used by bazel.sh)
  %(prog)s --install-dir /path/to/deps  # Custom deps directory
  %(prog)s --dry-run                    # Show what would be generated
  %(prog)s --target //fboss/agent:core  # Only convert one target
""",
    )
    parser.add_argument(
        "-r",
        "--repo-root",
        default=".",
        help="Repository root directory (default: current directory)",
    )
    parser.add_argument(
        "--install-dir",
        default=DEFAULT_INSTALL_DIR,
        help=f"Pre-built deps install directory (default: {DEFAULT_INSTALL_DIR})",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show what would be generated without writing files",
    )
    parser.add_argument(
        "--verbose",
        "-v",
        action="store_true",
        help="Show detailed output",
    )
    parser.add_argument(
        "--target",
        help="Only convert a specific target (e.g., //fboss/agent:core)",
    )
    parser.add_argument(
        "--generate-excludes",
        action="store_true",
        help="Annotate BUCK files with # bazelify: exclude based on cmake comparison",
    )
    parser.add_argument(
        "--no-getdeps-rules",
        dest="use_getdeps_rules",
        action="store_false",
        help="Use new_local_repository pointing to pre-built dirs instead of "
        "getdeps_repository rules that build deps on demand",
    )
    parser.set_defaults(use_getdeps_rules=True)
    parser.add_argument(
        "--scratch-path",
        default="/var/FBOSS/tmp_bld_dir",
        help="Scratch directory for getdeps builds",
    )
    parser.add_argument(
        "--cache-url",
        default="",
        help="HTTP cache URL for dep tarballs",
    )
    parser.add_argument(
        "--if-needed",
        action="store_true",
        help="Only regenerate if input files (BUCK, .bzl, manifests, patches) "
        "have changed since the last run. Uses a content checksum stored in "
        "MODULE.bazel to avoid unnecessary regeneration.",
    )
    return parser.parse_args()


def main():  # noqa: PLR0912, PLR0915
    args = _parse_args()
    repo_root = Path(args.repo_root).resolve()

    if args.generate_excludes:
        from bazelify_exclude import generate_excludes  # noqa: PLC0415

        generate_excludes(repo_root, dry_run=args.dry_run)
        return 0

    # Fast path: skip regeneration if inputs haven't changed.
    input_checksum = _compute_input_checksum(repo_root)
    if args.if_needed:
        stored = _read_stored_checksum(repo_root)
        if stored == input_checksum:
            # Still run the cheap orphan cleanup so stale BUILD.bazel files
            # left by a previous broken run (e.g. BUCK deleted while the
            # cleanup pass was broken) don't linger across --if-needed skips.
            # _write_refresh_compile_commands_build is content-idempotent
            # (no-op when content matches), so calling it here keeps the file
            # in sync if a user accidentally deleted it without bumping mtimes.
            if not args.dry_run:
                _cleanup_orphaned_build_files(repo_root)
                _write_refresh_compile_commands_build(repo_root)
            return 0
        if stored is None:
            print("No previous checksum found — regenerating")
        else:
            print("Input files changed — regenerating")

    # Step 1: Discover pre-built deps / build dep registry
    print("Discovering pre-built dependencies...")
    if args.use_getdeps_rules:
        print(
            "  Using getdeps_repository rules (default; use --no-getdeps-rules to disable)"
        )
        if not args.dry_run:
            _create_getdeps_build_files(repo_root)
        getdeps_graph = _discover_getdeps_graph(args.scratch_path)

        # In getdeps_repository mode deps are built lazily by Bazel repo rules
        # (not pre-built on disk).  Always use synthetic PrebuiltDep entries
        # derived from manifest metadata — never scan the local install dir.
        # Scanning stale installs would embed wrong paths or library types
        # (e.g., a shared glog from a previous build) into build_file_content.
        prebuilt_deps: dict[str, PrebuiltDep] = {}
        for dep_info in getdeps_graph:
            name = dep_info["name"]
            if dep_info["is_system"]:
                continue
            prebuilt_deps[name] = _synthetic_prebuilt_dep(name, dep_info)
        print(
            f"  Registered {len(prebuilt_deps)} deps as available (from getdeps graph)"
        )
    else:
        prebuilt_deps = discover_prebuilt_deps(args.install_dir)
        print(
            f"  Found {len(prebuilt_deps)} pre-built deps: "
            f"{', '.join(sorted(prebuilt_deps.keys()))}"
        )

    # Step 2: Parse all BUCK files
    print("Parsing BUCK files...")
    buck_parser = BuckParser(str(repo_root))
    targets_by_path = buck_parser.parse_all()
    total_targets = sum(len(t) for t in targets_by_path.values())
    print(f"  Found {total_targets} targets in {len(targets_by_path)} packages")
    if buck_parser._skipped_macros:
        print(
            f"  Skipped internal macros: {', '.join(sorted(buck_parser._skipped_macros))}"
        )

    # Step 3: Set up dependency resolver
    resolver = DepResolver(
        prebuilt_deps,
        buck_parser.all_targets,
        str(repo_root),
        bzl_include_pkgs=set(buck_parser._bzl_includes.keys()),
    )
    resolver._excluded_targets = buck_parser._excluded_target_names

    # Step 4: Generate MODULE.bazel
    print("Generating MODULE.bazel...")
    if args.use_getdeps_rules:
        module_content = generate_module_bazel_getdeps(
            prebuilt_deps,
            getdeps_graph,
            args.scratch_path,
            args.cache_url,
        )
    else:
        module_content = generate_module_bazel(prebuilt_deps)
    module_file = repo_root / "MODULE.bazel"

    if args.dry_run:
        print("  [DRY RUN] Would write MODULE.bazel")
    else:
        # Embed input checksum so --if-needed can skip future runs.
        checksum_line = f"{_INPUT_CHECKSUM_PREFIX}{input_checksum}"
        module_content = module_content.replace(
            "# AUTO-GENERATED by bazelify.py - DO NOT EDIT",
            f"# AUTO-GENERATED by bazelify.py - DO NOT EDIT\n{checksum_line}",
            1,
        )
        module_file.write_text(module_content + "\n")
        print(f"  Wrote {module_file}")

        # Also write an empty WORKSPACE.bazel (required by Bazel)
        workspace_file = repo_root / "WORKSPACE.bazel"
        workspace_file.write_text("# Empty - using bzlmod (MODULE.bazel)\n")
        print(f"  Wrote {workspace_file}")

    # Step 4b: Detect folly's F14IntrinsicsMode and write `folly.bazelrc`
    # folly's F14 containers use a compile-time ABI check (F14LinkCheck) that
    # must match between folly and its consumers.  Prefer inspecting the built
    # libfolly.a; fall back to CPU feature detection for fresh builds where
    # deps haven't been built yet.
    if not args.dry_run:
        _write_bazelrc_folly(repo_root, prebuilt_deps.get("folly"))

    # Step 5: Generate BUILD.bazel files
    print("Generating BUILD.bazel files...")
    generator = BuildFileGenerator(
        resolver, str(repo_root), bzl_includes=buck_parser._bzl_includes
    )

    if args.target:
        # Only generate for a specific target
        target_path = (
            args.target.rsplit(":", 1)[0] if ":" in args.target else args.target
        )
        filtered = {k: v for k, v in targets_by_path.items() if k == target_path}
        if not filtered:
            print(f"  Error: Target path {target_path} not found")
            return 1
        generator.generate_all(filtered, exclude_all_pkgs=buck_parser.exclude_all_pkgs)
    elif args.dry_run:
        print(
            f"  [DRY RUN] Would generate BUILD.bazel files for {len(targets_by_path)} packages"
        )
    else:
        generator.generate_all(
            targets_by_path, exclude_all_pkgs=buck_parser.exclude_all_pkgs
        )

    print(f"  Generated {generator.generated_count} BUILD.bazel files")

    if generator.skipped_targets:
        print(f"\nSkipped {len(generator.skipped_targets)} targets:")
        for t in generator.skipped_targets[:20]:
            print(f"  {t}")
        if len(generator.skipped_targets) > 20:
            print(f"  ... and {len(generator.skipped_targets) - 20} more")

    if resolver.warnings:
        unique_warnings = sorted(set(resolver.warnings))
        print(f"\nDependency warnings ({len(unique_warnings)}):")
        for w in unique_warnings[:30]:
            print(w)
        if len(unique_warnings) > 30:
            print(f"  ... and {len(unique_warnings) - 30} more")

    # Step 6: Create package markers if needed
    if not args.dry_run:
        # Root BUILD.bazel is needed so that getdeps_dep.bzl can resolve
        # Label("@//:MODULE.bazel") to find the repo root.  If the repo
        # doesn't ship a committed root BUILD.bazel, create a minimal one.
        root_build = repo_root / "BUILD.bazel"
        if not root_build.exists():
            root_build.write_text("# Root package marker for Bazel\n")

        # fboss/oss/refresh/BUILD.bazel: not committed, kept out of
        # fboss/build_defs/ so the legacy OSS Bazel 8 build (Monobuild) — which
        # chokes on the @hedron_compile_commands load — never sees this file.
        _write_refresh_compile_commands_build(repo_root)

    print("\nDone!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
