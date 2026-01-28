#!/usr/bin/env python3
"""
Tool to detect missing dependencies between Buck and cmake builds.

This tool:
1. Parses all BUCK files to extract targets and their dependencies
2. Parses all cmake files to extract targets and their dependencies
3. Correlates cmake targets with Buck targets by matching source files
4. Reports missing dependencies in cmake files
"""

import argparse
import os
import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional


@dataclass
class BuckTarget:
    """Represents a Buck target (cpp_library, thrift_library, etc.)"""

    name: str
    buck_path: str  # e.g., "//fboss/fsdb/common"
    full_name: str  # e.g., "//fboss/fsdb/common:utils"
    target_type: str  # "cpp_library", "thrift_library", etc.
    srcs: list[str] = field(default_factory=list)
    headers: list[str] = field(default_factory=list)
    deps: list[str] = field(default_factory=list)
    exported_deps: list[str] = field(default_factory=list)
    thrift_srcs: list[str] = field(default_factory=list)
    file_path: str = ""  # Actual BUCK file path


@dataclass
class CmakeTarget:
    """Represents a cmake target (add_library, add_fbthrift_cpp_library, etc.)"""

    name: str
    target_type: str  # "library", "thrift", "executable"
    srcs: list[str] = field(default_factory=list)
    deps: list[str] = field(default_factory=list)
    thrift_deps: list[str] = field(default_factory=list)
    file_path: str = ""  # The cmake file where this target is defined


@dataclass
class TargetMatch:
    """Represents a match between a cmake target and a Buck target"""

    cmake_target: CmakeTarget
    buck_target: BuckTarget
    matching_srcs: list[str] = field(default_factory=list)
    match_score: float = 0.0  # 0-1, 1 being perfect match


@dataclass
class MissingDependency:
    """Represents a missing dependency in cmake"""

    cmake_target: CmakeTarget
    buck_target: BuckTarget
    buck_dep: str  # The Buck dependency that's missing
    suggested_cmake_dep: Optional[str] = None  # Suggested cmake target to add


class BuckParser:
    """Parser for BUCK files"""

    def __init__(self, repo_root: str):
        self.repo_root = Path(repo_root)
        self.targets: dict[str, BuckTarget] = {}  # full_name -> target

    def parse_all(self) -> dict[str, BuckTarget]:
        """Parse all BUCK files in the repository"""
        for buck_file in self.repo_root.rglob("BUCK"):
            self._parse_file(buck_file)
        return self.targets

    def _parse_file(self, file_path: Path):
        """Parse a single BUCK file"""
        try:
            with open(file_path, "r") as f:
                content = f.read()
        except Exception as e:
            print(f"Warning: Could not read {file_path}: {e}")
            return

        # Get the buck path from file location
        rel_path = file_path.parent.relative_to(self.repo_root)
        buck_path = "//" + str(rel_path).replace("\\", "/")

        # Parse cpp_library targets
        self._parse_cpp_library(content, buck_path, str(file_path))

        # Parse thrift_library targets
        self._parse_thrift_library(content, buck_path, str(file_path))

    def _extract_string_list(self, content: str, key: str) -> list[str]:
        """Extract a list of strings from a Buck stanza"""
        # Match patterns like: key = ["item1", "item2"] or key = [\n"item1",\n"item2"\n]
        # Use word boundary or start-of-line/whitespace to ensure 'deps' doesn't match
        # inside 'cpp2_deps' or 'exported_deps'
        pattern = rf"(?:^|[\s,\n])({key})\s*=\s*\[(.*?)\]"
        match = re.search(pattern, content, re.DOTALL | re.MULTILINE)
        if not match:
            return []

        list_content = match.group(2)
        # Extract all quoted strings
        items = re.findall(r'"([^"]*)"', list_content)
        return items

    def _extract_deps_list(self, content: str, key: str) -> list[str]:
        """Extract a list of dependencies from a Buck stanza"""
        return self._extract_string_list(content, key)

    def _parse_cpp_library(self, content: str, buck_path: str, file_path: str):
        """Parse cpp_library declarations"""
        # Find all cpp_library blocks
        pattern = r'cpp_library\s*\(\s*name\s*=\s*"([^"]+)"'
        for match in re.finditer(pattern, content):
            name = match.group(1)
            start = match.start()

            # Find the end of this target (matching parenthesis)
            block_content = self._extract_block(content, start)
            if not block_content:
                continue

            target = BuckTarget(
                name=name,
                buck_path=buck_path,
                full_name=f"{buck_path}:{name}",
                target_type="cpp_library",
                srcs=self._extract_string_list(block_content, "srcs"),
                headers=self._extract_string_list(block_content, "headers"),
                deps=self._extract_deps_list(block_content, "deps"),
                exported_deps=self._extract_deps_list(block_content, "exported_deps"),
                file_path=file_path,
            )
            self.targets[target.full_name] = target

    def _parse_thrift_library(self, content: str, buck_path: str, file_path: str):
        """Parse thrift_library declarations"""
        pattern = r'thrift_library\s*\(\s*name\s*=\s*"([^"]+)"'
        for match in re.finditer(pattern, content):
            name = match.group(1)
            start = match.start()

            block_content = self._extract_block(content, start)
            if not block_content:
                continue

            # thrift_srcs is a dict like {"file.thrift": [...]}
            thrift_srcs = self._extract_thrift_srcs(block_content)

            target = BuckTarget(
                name=name,
                buck_path=buck_path,
                full_name=f"{buck_path}:{name}",
                target_type="thrift_library",
                deps=self._extract_deps_list(block_content, "deps"),
                thrift_srcs=thrift_srcs,
                file_path=file_path,
            )
            self.targets[target.full_name] = target

    def _extract_thrift_srcs(self, content: str) -> list[str]:
        """Extract thrift source files from thrift_srcs dict"""
        pattern = r"thrift_srcs\s*=\s*\{([^}]*)\}"
        match = re.search(pattern, content, re.DOTALL)
        if not match:
            return []
        # Extract filenames from the dict keys
        dict_content = match.group(1)
        files = re.findall(r'"([^"]+\.thrift)"', dict_content)
        return files

    def _extract_block(self, content: str, start: int) -> Optional[str]:
        """Extract a complete block starting from a function call"""
        # Find the opening parenthesis
        paren_start = content.find("(", start)
        if paren_start == -1:
            return None

        depth = 1
        pos = paren_start + 1
        while pos < len(content) and depth > 0:
            if content[pos] == "(":
                depth += 1
            elif content[pos] == ")":
                depth -= 1
            pos += 1

        if depth == 0:
            return content[paren_start:pos]
        return None


class CmakeParser:
    """Parser for cmake files"""

    def __init__(self, repo_root: str):
        self.repo_root = Path(repo_root)
        self.targets: dict[str, CmakeTarget] = {}  # name -> target
        self.variables: dict[str, list[str]] = {}  # cmake variable -> values
        self.duplicate_errors: list[str] = []  # List of duplicate target errors

    def _add_target(self, name: str, target: CmakeTarget):
        """Add a target, reporting error if it already exists"""
        if name in self.targets:
            existing = self.targets[name]
            error = (
                f"ERROR: Duplicate cmake target '{name}' defined in:\n"
                f"  First:  {existing.file_path}\n"
                f"  Second: {target.file_path}"
            )
            self.duplicate_errors.append(error)
            print(error)
        self.targets[name] = target

    def parse_all(self) -> dict[str, CmakeTarget]:
        """Parse all cmake files in cmake/ directory and CMakeLists.txt"""
        # Parse the main CMakeLists.txt first
        cmakelists = self.repo_root / "CMakeLists.txt"
        if cmakelists.exists():
            self._parse_file(cmakelists)

        # Then parse all cmake files in the cmake/ directory
        cmake_dir = self.repo_root / "cmake"
        for cmake_file in cmake_dir.rglob("*.cmake"):
            self._parse_file(cmake_file)
        return self.targets

    def _parse_file(self, file_path: Path):
        """Parse a single cmake file"""
        try:
            with open(file_path, "r") as f:
                content = f.read()
        except Exception as e:
            print(f"Warning: Could not read {file_path}: {e}")
            return

        # Parse set() commands first to capture variables
        self._parse_set_commands(content)

        # Parse add_library
        self._parse_add_library(content, str(file_path))

        # Parse add_fbthrift_cpp_library
        self._parse_add_fbthrift_cpp_library(content, str(file_path))

        # Parse add_executable
        self._parse_add_executable(content, str(file_path))

        # Parse target_link_libraries to get dependencies
        self._parse_target_link_libraries(content)

    def _parse_set_commands(self, content: str):
        """Parse set() commands to capture variable definitions"""
        # Match set(VAR_NAME value1 value2 ...) or set(VAR_NAME\n  value1\n  value2\n)
        pattern = r"set\s*\(\s*(\w+)\s+([^)]+)\)"
        for match in re.finditer(pattern, content, re.IGNORECASE):
            var_name = match.group(1)
            values_block = match.group(2)
            # Extract values (split by whitespace/newlines, filter empty/comments)
            values = []
            for line in values_block.split("\n"):
                for token in line.split():
                    token = token.strip()
                    if token and not token.startswith("#"):
                        values.append(token)
            if values:
                self.variables[var_name] = values

    def _expand_variables(self, deps: list[str]) -> list[str]:
        """Expand cmake ${VAR} references in dependency list"""
        expanded = []
        for dep in deps:
            dep = dep.strip()
            if not dep:
                continue
            # Check if this is a variable reference like ${var_name}
            var_match = re.match(r"\$\{(\w+)\}", dep)
            if var_match:
                var_name = var_match.group(1)
                if var_name in self.variables:
                    expanded.extend(self.variables[var_name])
                # else: unknown variable, skip it
            else:
                expanded.append(dep)
        return expanded

    def _parse_add_library(self, content: str, file_path: str):
        """Parse add_library calls"""
        # Match add_library(name\n  file1.cpp\n  file2.cpp\n)
        pattern = r"add_library\s*\(\s*(\w+)\s*\n([^)]*)\)"
        for match in re.finditer(pattern, content):
            name = match.group(1)
            files_block = match.group(2)
            # Extract file paths
            srcs = self._extract_source_files(files_block)

            target = CmakeTarget(
                name=name, target_type="library", srcs=srcs, file_path=file_path
            )
            self._add_target(name, target)

    def _parse_add_executable(self, content: str, file_path: str):
        """Parse add_executable calls"""
        pattern = r"add_executable\s*\(\s*(\w+)\s*\n([^)]*)\)"
        for match in re.finditer(pattern, content):
            name = match.group(1)
            files_block = match.group(2)
            srcs = self._extract_source_files(files_block)

            target = CmakeTarget(
                name=name, target_type="executable", srcs=srcs, file_path=file_path
            )
            self._add_target(name, target)

    def _parse_add_fbthrift_cpp_library(self, content: str, file_path: str):
        """Parse add_fbthrift_cpp_library calls"""
        # Match the target name and thrift file
        pattern = r"add_fbthrift_cpp_library\s*\(\s*(\w+)\s*\n\s*([^\s]+\.thrift)"
        for match in re.finditer(pattern, content):
            name = match.group(1)
            thrift_file = match.group(2)

            # Find the full block to extract DEPENDS
            block_start = match.start()
            block = self._extract_cmake_block(content, block_start)
            deps = []
            if block:
                deps = self._extract_thrift_depends(block)

            target = CmakeTarget(
                name=name,
                target_type="thrift",
                srcs=[thrift_file],
                thrift_deps=deps,
                file_path=file_path,
            )
            self._add_target(name, target)

    def _extract_thrift_depends(self, block: str) -> list[str]:
        """Extract DEPENDS from a thrift cmake block"""
        # Look for DEPENDS section
        pattern = r"DEPENDS\s*\n((?:\s+\w+\s*\n)*)"
        match = re.search(pattern, block)
        if not match:
            return []
        deps_block = match.group(1)
        return [d.strip() for d in deps_block.split("\n") if d.strip()]

    def _extract_cmake_block(self, content: str, start: int) -> Optional[str]:
        """Extract a cmake block from ( to )"""
        paren_start = content.find("(", start)
        if paren_start == -1:
            return None

        depth = 1
        pos = paren_start + 1
        while pos < len(content) and depth > 0:
            if content[pos] == "(":
                depth += 1
            elif content[pos] == ")":
                depth -= 1
            pos += 1

        if depth == 0:
            return content[paren_start:pos]
        return None

    def _parse_target_link_libraries(self, content: str):
        """Parse target_link_libraries to associate dependencies with targets"""
        # Match both multi-line and single-line formats
        pattern = r"target_link_libraries\s*\(\s*(\w+)\s+([^)]*)\)"
        for match in re.finditer(pattern, content):
            target_name = match.group(1)
            deps_block = match.group(2)

            if target_name in self.targets:
                # Parse deps from the block (split by whitespace/newlines)
                raw_deps = []
                for line in deps_block.split("\n"):
                    for token in line.split():
                        token = token.strip()
                        if token and not token.startswith("#"):
                            raw_deps.append(token)
                # Expand any ${VAR} references
                deps = self._expand_variables(raw_deps)
                # Extend existing deps (don't overwrite - there may be multiple
                # target_link_libraries calls for the same target)
                self.targets[target_name].deps.extend(deps)

    def _extract_source_files(self, block: str) -> list[str]:
        """Extract source file paths from a cmake block"""
        files = []
        for line in block.split("\n"):
            line = line.strip()
            # Skip comments and empty lines
            if not line or line.startswith("#"):
                continue
            # Skip cmake keywords
            if line in ("PUBLIC", "PRIVATE", "INTERFACE"):
                continue
            files.append(line)
        return files


class DependencyAnalyzer:
    """Analyzes dependencies between Buck and cmake targets"""

    def __init__(self, repo_root: str):
        self.repo_root = Path(repo_root)
        self.buck_parser = BuckParser(repo_root)
        self.cmake_parser = CmakeParser(repo_root)
        self.buck_targets: dict[str, BuckTarget] = {}
        self.cmake_targets: dict[str, CmakeTarget] = {}
        self.matches: list[TargetMatch] = []
        # Maps to help correlate Buck deps to cmake deps
        self.buck_to_cmake: dict[str, str] = {}  # buck full_name -> cmake name

    def analyze(self) -> list[MissingDependency]:
        """Run the full analysis and return missing dependencies"""
        print("Parsing BUCK files...")
        self.buck_targets = self.buck_parser.parse_all()
        print(f"  Found {len(self.buck_targets)} Buck targets")

        print("Parsing cmake files...")
        self.cmake_targets = self.cmake_parser.parse_all()
        print(f"  Found {len(self.cmake_targets)} cmake targets")

        print("Matching targets by source files...")
        self.matches = self._match_targets()
        print(f"  Found {len(self.matches)} matches")

        print("Building Buck-to-cmake dependency mapping...")
        self._build_dep_mapping()

        print("Analyzing missing dependencies...")
        missing = self._find_missing_deps()
        return missing

    def _normalize_path(self, path: str, buck_path: str = "") -> str:
        """Normalize a source file path to a canonical form for comparison"""
        # Remove leading fboss/ if present since Buck files use relative paths
        path = path.strip()

        # For cmake paths like 'fboss/fsdb/common/Utils.cpp'
        if path.startswith("fboss/"):
            return path

        # For Buck paths which are relative to their directory
        if buck_path:
            # buck_path is like "//fboss/fsdb/common"
            dir_path = buck_path.lstrip("/")
            return f"{dir_path}/{path}"

        return path

    def _match_targets(self) -> list[TargetMatch]:
        """Match cmake targets to Buck targets based on source files"""
        matches = []

        for _, cmake_target in self.cmake_targets.items():
            if not cmake_target.srcs:
                continue

            # Normalize cmake source files
            cmake_srcs = set()
            for src in cmake_target.srcs:
                # Only consider .cpp, .h, .thrift files
                if src.endswith((".cpp", ".h", ".thrift")):
                    cmake_srcs.add(src)

            if not cmake_srcs:
                continue

            best_match: Optional[BuckTarget] = None
            best_score = 0.0
            best_matching_srcs: list[str] = []
            same_name_match: Optional[BuckTarget] = None
            same_name_matching_srcs: list[str] = []

            for buck_target in self.buck_targets.values():
                # Normalize Buck source files
                buck_srcs = set()
                for src in buck_target.srcs:
                    normalized = self._normalize_path(src, buck_target.buck_path)
                    buck_srcs.add(normalized)
                for src in buck_target.headers:
                    normalized = self._normalize_path(src, buck_target.buck_path)
                    buck_srcs.add(normalized)
                for src in buck_target.thrift_srcs:
                    # thrift files need directory context
                    normalized = self._normalize_path(src, buck_target.buck_path)
                    buck_srcs.add(normalized)

                if not buck_srcs:
                    continue

                # Find intersection
                matching = cmake_srcs.intersection(buck_srcs)
                if not matching:
                    continue

                # Score based on Jaccard similarity
                union = cmake_srcs.union(buck_srcs)
                score = len(matching) / len(union) if union else 0

                # If names match exactly, remember this as a strong candidate
                if buck_target.name == cmake_target.name:
                    same_name_match = buck_target
                    same_name_matching_srcs = list(matching)

                if score > best_score:
                    best_score = score
                    best_match = buck_target
                    best_matching_srcs = list(matching)

            # Prefer same-name match over highest-score match if both exist
            if same_name_match:
                best_match = same_name_match
                best_matching_srcs = same_name_matching_srcs
                # Recalculate score for the same-name match
                buck_srcs = set()
                for src in same_name_match.srcs:
                    buck_srcs.add(self._normalize_path(src, same_name_match.buck_path))
                for src in same_name_match.headers:
                    buck_srcs.add(self._normalize_path(src, same_name_match.buck_path))
                for src in same_name_match.thrift_srcs:
                    buck_srcs.add(self._normalize_path(src, same_name_match.buck_path))
                union = cmake_srcs.union(buck_srcs)
                best_score = (
                    len(set(same_name_matching_srcs)) / len(union) if union else 0
                )

            if best_match and best_score > 0:
                matches.append(
                    TargetMatch(
                        cmake_target=cmake_target,
                        buck_target=best_match,
                        matching_srcs=best_matching_srcs,
                        match_score=best_score,
                    )
                )

        return matches

    def _build_dep_mapping(self):
        """Build a mapping from Buck target names to cmake target names.

        When multiple cmake targets match the same Buck target, prefer
        the cmake target with the same name as the Buck target.

        For thrift libraries, also map sub-targets like -cpp2-types, -cpp2-services
        to the corresponding cmake thrift target.
        """
        # Track which Buck targets have same-name cmake matches
        same_name_matches: dict[str, str] = {}  # buck_full_name -> cmake_name

        for match in self.matches:
            if match.buck_target.name == match.cmake_target.name:
                same_name_matches[match.buck_target.full_name] = match.cmake_target.name

        for match in self.matches:
            buck_full = match.buck_target.full_name
            # If there's a same-name match for this Buck target, use it
            if buck_full in same_name_matches:
                cmake_name = same_name_matches[buck_full]
            else:
                cmake_name = match.cmake_target.name

            self.buck_to_cmake[buck_full] = cmake_name
            # Also map short form like ":name" within same BUCK file
            short_name = f":{match.buck_target.name}"
            self.buck_to_cmake[short_name] = cmake_name

            # For thrift libraries, also map sub-targets like -cpp2-types
            # Buck thrift_library generates sub-targets like:
            #   //path:name-cpp2-types, //path:name-cpp2-services, etc.
            # These should all map to the same cmake thrift target
            if match.buck_target.target_type == "thrift_library":
                thrift_suffixes = [
                    "-cpp2-types",
                    "-cpp2-services",
                    "-cpp2",
                    "-py",
                    "-py3",
                    "-rust",
                    "-go",
                ]
                buck_path = match.buck_target.buck_path
                buck_name = match.buck_target.name
                for suffix in thrift_suffixes:
                    sub_target_full = f"{buck_path}:{buck_name}{suffix}"
                    sub_target_short = f":{buck_name}{suffix}"
                    self.buck_to_cmake[sub_target_full] = cmake_name
                    self.buck_to_cmake[sub_target_short] = cmake_name

    def _resolve_buck_dep(self, dep: str, buck_path: str) -> str:
        """Resolve a Buck dependency to its full form"""
        if dep.startswith(":"):
            # Local reference within same BUCK file
            return f"{buck_path}{dep}"
        elif dep.startswith("//"):
            return dep
        return dep

    def _get_transitive_cmake_deps(
        self, target_name: str, visited: Optional[set[str]] = None
    ) -> set[str]:
        """Get all transitive dependencies of a cmake target"""
        if visited is None:
            visited = set()
        if target_name in visited:
            return set()
        visited.add(target_name)

        target = self.cmake_targets.get(target_name)
        if not target:
            return set()

        all_deps = set(target.deps) | set(target.thrift_deps)
        for dep in list(all_deps):
            # Pass visited by reference to avoid re-exploring same nodes
            all_deps |= self._get_transitive_cmake_deps(dep, visited)
        return all_deps

    def _find_missing_deps(self) -> list[MissingDependency]:
        """Find dependencies that exist in Buck but are missing in cmake"""
        missing = []

        for match in self.matches:
            cmake_target = match.cmake_target
            buck_target = match.buck_target

            # Get all Buck dependencies
            all_buck_deps = set(buck_target.deps) | set(buck_target.exported_deps)

            # Get cmake dependencies (including transitive)
            cmake_deps = set(cmake_target.deps) | set(cmake_target.thrift_deps)
            transitive_cmake_deps = self._get_transitive_cmake_deps(cmake_target.name)

            for buck_dep in all_buck_deps:
                # Resolve the Buck dep
                full_buck_dep = self._resolve_buck_dep(buck_dep, buck_target.buck_path)

                # Check if we have a cmake equivalent
                cmake_equiv = self.buck_to_cmake.get(full_buck_dep)

                if cmake_equiv:
                    # Check if cmake has this dependency (directly or transitively)
                    if (
                        cmake_equiv not in cmake_deps
                        and cmake_equiv not in transitive_cmake_deps
                    ):
                        missing.append(
                            MissingDependency(
                                cmake_target=cmake_target,
                                buck_target=buck_target,
                                buck_dep=buck_dep,
                                suggested_cmake_dep=cmake_equiv,
                            )
                        )

        return missing


def print_report(
    missing_deps: list[MissingDependency],
    matches: list[TargetMatch],
    verbose: bool = False,
):
    """Print a report of the analysis"""
    if verbose:
        print("\n" + "=" * 80)
        print("TARGET MATCHES (cmake -> Buck)")
        print("=" * 80)
        for match in sorted(matches, key=lambda m: m.cmake_target.name):
            print(f"\n{match.cmake_target.name} -> {match.buck_target.full_name}")
            print(f"  Match score: {match.match_score:.2%}")
            print(f"  cmake file: {match.cmake_target.file_path}")
            print(f"  BUCK file: {match.buck_target.file_path}")
            if match.matching_srcs:
                print(f"  Matching sources: {', '.join(match.matching_srcs[:3])}")
                if len(match.matching_srcs) > 3:
                    print(f"    ... and {len(match.matching_srcs) - 3} more")

    print("\n" + "=" * 80)
    print("MISSING DEPENDENCIES")
    print("=" * 80)

    if not missing_deps:
        print("\nNo missing dependencies found!")
        return

    # Group by cmake file
    by_file: dict[str, list[MissingDependency]] = {}
    for md in missing_deps:
        path = md.cmake_target.file_path
        if path not in by_file:
            by_file[path] = []
        by_file[path].append(md)

    for file_path, deps in sorted(by_file.items()):
        print(f"\n{file_path}:")
        for md in deps:
            print(f"  Target: {md.cmake_target.name}")
            print(f"    Missing: {md.suggested_cmake_dep}")
            print(f"    (from Buck dep: {md.buck_dep})")
            print(f"    Buck target: {md.buck_target.full_name}")


def main():
    parser = argparse.ArgumentParser(
        description="Check for missing cmake dependencies compared to Buck",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                    # Analyze current directory
  %(prog)s -r /path/to/fboss  # Analyze specific repository
  %(prog)s -v                 # Verbose output with match details
  %(prog)s --target foo       # Only analyze target 'foo'
""",
    )
    parser.add_argument(
        "-r",
        "--repo-root",
        default=".",
        help="Repository root directory (default: current directory)",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Show verbose output including all matches",
    )
    parser.add_argument("--target", help="Only analyze a specific cmake target")
    parser.add_argument(
        "--list-matches",
        action="store_true",
        help="List all cmake-to-Buck target matches",
    )
    parser.add_argument(
        "--list-buck", action="store_true", help="List all Buck targets found"
    )
    parser.add_argument(
        "--list-cmake", action="store_true", help="List all cmake targets found"
    )

    args = parser.parse_args()

    # Resolve the repo root
    repo_root = Path(args.repo_root).resolve()
    if not (repo_root / "cmake").is_dir():
        print(f"Error: {repo_root}/cmake directory not found")
        print("Make sure you're running from the fboss repository root")
        return 1

    analyzer = DependencyAnalyzer(str(repo_root))
    missing = analyzer.analyze()

    # Handle list commands
    if args.list_buck:
        print("\nBuck targets:")
        for name, target in sorted(analyzer.buck_targets.items()):
            print(f"  {name} ({target.target_type})")
            if target.srcs:
                print(f"    srcs: {target.srcs[:3]}...")
        return 0

    if args.list_cmake:
        print("\ncmake targets:")
        for name, target in sorted(analyzer.cmake_targets.items()):
            print(f"  {name} ({target.target_type})")
            print(f"    file: {target.file_path}")
            if target.srcs:
                print(f"    srcs: {target.srcs[:3]}...")
        return 0

    if args.list_matches:
        print("\nTarget matches (cmake -> Buck):")
        for match in sorted(analyzer.matches, key=lambda m: m.cmake_target.name):
            print(f"  {match.cmake_target.name} -> {match.buck_target.full_name}")
            print(f"    score: {match.match_score:.2%}")
        return 0

    # Filter by target if specified
    if args.target:
        missing = [m for m in missing if m.cmake_target.name == args.target]

    print_report(missing, analyzer.matches, args.verbose)

    # Return non-zero if there are missing dependencies or duplicate targets
    has_duplicates = len(analyzer.cmake_parser.duplicate_errors) > 0
    return 1 if missing or has_duplicates else 0


if __name__ == "__main__":
    exit(main())
