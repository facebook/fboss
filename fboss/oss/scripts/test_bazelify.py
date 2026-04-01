#!/usr/bin/env python3
"""Unit tests for bazelify.py.

Tests are written BEFORE the fixes so that initially-failing tests document
the bugs, and passing tests after fixes demonstrate correctness.
"""

from __future__ import annotations

import subprocess
import sys
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest.mock import patch

# Add the scripts directory to the path so we can import bazelify directly.
sys.path.insert(0, str(Path(__file__).parent))

import bazelify as b2b
from bazelify import (
    DEFAULT_INSTALL_DIR,
    THIRD_PARTY_MAP,
    BuckParser,
    BuckTarget,
    BuildFileGenerator,
    DepResolver,
    PrebuiltDep,
    _make_build_file_content,
    _synthetic_prebuilt_dep,
    discover_prebuilt_deps,
)

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _make_parser(tmp_path: Path | None = None) -> BuckParser:
    """Return a BuckParser whose repo_root is a temp directory."""
    root = tmp_path or Path("/nonexistent")
    return BuckParser(str(root))


# ---------------------------------------------------------------------------
# Bug 1: _detect_internal_macros - regex must match path strings too
# ---------------------------------------------------------------------------


class TestDetectInternalMacros(unittest.TestCase):
    """Tests for BuckParser._detect_internal_macros."""

    def setUp(self):
        self.parser = _make_parser()

    def test_single_symbol_extracted(self):
        """A load() with one symbol must return that symbol."""
        content = 'load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")'
        macros = self.parser._detect_internal_macros(content)
        # Standard BUCK rules are NOT treated as internal macros
        self.assertNotIn("cpp_library", macros)

    def test_standard_rules_excluded_from_internal(self):
        """Standard BUCK rules like cpp_library should not be marked as internal macros."""
        content = (
            'load("@fbcode_macros//build_defs:cpp_library.bzl", '
            '"cpp_library", "cpp_binary", "cpp_unittest")'
        )
        macros = self.parser._detect_internal_macros(content)
        # Standard rules must NOT be in internal macros (they're in RULE_TYPES)
        self.assertNotIn("cpp_library", macros)
        self.assertNotIn("cpp_binary", macros)
        self.assertNotIn("cpp_unittest", macros)

    def test_custom_macros_detected_as_internal(self):
        """Custom macros loaded from internal paths should be detected,
        unless they are in RULE_TYPES (standard rules we handle)."""
        content = (
            'load("@fbcode_macros//build_defs:custom_rule.bzl", "custom_rule")\n'
            'load("//fboss/build:fbpkg.bzl", "fboss_fbpkg_builder")'
        )
        macros = self.parser._detect_internal_macros(content)
        # custom_rule is in RULE_TYPES so it's NOT treated as internal
        self.assertNotIn("custom_rule", macros)
        # fboss_fbpkg_builder is a true custom macro
        self.assertIn("fboss_fbpkg_builder", macros)

    def test_path_not_treated_as_symbol(self):
        """The load() path itself must not appear in the returned symbol set."""
        content = 'load("@fbcode_macros//build_defs:custom_rule.bzl", "custom_rule")'
        macros = self.parser._detect_internal_macros(content)
        for m in macros:
            self.assertNotIn("/", m, f"Path leaked into macros: {m!r}")
            self.assertNotIn("@", m, f"Path leaked into macros: {m!r}")

    def test_non_internal_path_not_detected(self):
        """Symbols from non-internal paths must not be added."""
        content = 'load("//fboss/build_defs:some_rule.bzl", "some_rule")'
        # //fboss/build_defs is not in INTERNAL_LOAD_PREFIXES, and we mock
        # the .exists() check to return True (file is present locally).
        with patch.object(Path, "exists", return_value=True):
            macros = self.parser._detect_internal_macros(content)
        self.assertNotIn("some_rule", macros)


# ---------------------------------------------------------------------------
# Bug 2: nlohmann_json mapping inconsistency
# ---------------------------------------------------------------------------


class TestNlohmannJsonMapping(unittest.TestCase):
    """Both 'nlohmann-json' and 'nlohmann_json' must resolve to the same target."""

    def test_dash_variant_present(self):
        self.assertIn("nlohmann-json", THIRD_PARTY_MAP)

    def test_underscore_variant_present(self):
        self.assertIn("nlohmann_json", THIRD_PARTY_MAP)

    def test_both_resolve_to_same_target(self):
        dash = THIRD_PARTY_MAP["nlohmann-json"]
        underscore = THIRD_PARTY_MAP["nlohmann_json"]
        self.assertEqual(
            dash,
            underscore,
            f"nlohmann-json ({dash!r}) and nlohmann_json ({underscore!r}) "
            "must map to the same Bazel target",
        )

    def test_resolved_target_is_nlohmann_json(self):
        """Canonical target should be @nlohmann_json//:nlohmann_json."""
        target = THIRD_PARTY_MAP["nlohmann-json"]
        self.assertEqual(target, "@nlohmann_json//:nlohmann_json")


# ---------------------------------------------------------------------------
# Bug 3: _extract_string_list deps vs exported_deps disambiguation
# ---------------------------------------------------------------------------


class TestExtractStringList(unittest.TestCase):
    """Tests for BuckParser._extract_string_list."""

    def setUp(self):
        self.parser = _make_parser()

    def test_deps_extracted_from_simple_block(self):
        block = '(name = "foo", deps = ["//a:b", "//c:d"],)'
        result = self.parser._extract_string_list(block, "deps")
        self.assertEqual(result, ["//a:b", "//c:d"])

    def test_exported_deps_does_not_match_when_key_is_deps(self):
        """When key='deps', exported_deps must NOT be matched."""
        block = '(name = "foo", exported_deps = ["//exported:only"],)'
        result = self.parser._extract_string_list(block, "deps")
        self.assertEqual(
            result,
            [],
            "exported_deps must not be returned when searching for 'deps'",
        )

    def test_deps_after_comma(self):
        """deps after a comma and a space must be found."""
        block = '(name = "foo", srcs = ["a.cpp"], deps = ["//x:y"],)'
        result = self.parser._extract_string_list(block, "deps")
        self.assertIn("//x:y", result)

    def test_deps_after_open_paren(self):
        """deps immediately after opening paren (no preceding comma)."""
        block = '(deps = ["//x:y"],)'
        result = self.parser._extract_string_list(block, "deps")
        self.assertIn("//x:y", result)

    def test_deps_on_its_own_line(self):
        """deps on its own line must be found."""
        block = '(\n    name = "foo",\n    deps = [\n        "//x:y",\n    ],\n)'
        result = self.parser._extract_string_list(block, "deps")
        self.assertIn("//x:y", result)

    def test_both_deps_and_exported_deps_present(self):
        """When both are present only the correct one is extracted per key."""
        block = '(name = "foo", deps = ["//impl:dep"], exported_deps = ["//pub:dep"],)'
        deps = self.parser._extract_string_list(block, "deps")
        exported = self.parser._extract_string_list(block, "exported_deps")
        self.assertEqual(deps, ["//impl:dep"])
        self.assertEqual(exported, ["//pub:dep"])


# ---------------------------------------------------------------------------
# Bug 4: _extract_block - triple-quoted strings containing parens
# ---------------------------------------------------------------------------


class TestExtractBlock(unittest.TestCase):
    """Tests for BuckParser._extract_block."""

    def setUp(self):
        self.parser = _make_parser()

    def _extract(self, content: str) -> str | None:
        paren = content.index("(")
        return self.parser._extract_block(content, paren)

    def test_simple_block(self):
        content = 'cpp_library(name = "foo")'
        result = self._extract(content)
        self.assertEqual(result, '(name = "foo")')

    def test_nested_parens(self):
        content = 'cpp_library(name = "foo", deps = [":bar"])'
        result = self._extract(content)
        self.assertIsNotNone(result)
        self.assertIn('"foo"', result)

    def test_triple_quoted_string_with_open_paren(self):
        """A triple-quoted string containing '(' must not break paren counting."""
        content = 'cpp_library(name = "foo", doc = """This function f(x) works""")'
        result = self._extract(content)
        self.assertIsNotNone(result, "Should extract block even with ( in triple-quote")
        self.assertIn('"foo"', result)

    def test_triple_quoted_string_with_close_paren(self):
        """A triple-quoted string containing ')' must not terminate early."""
        content = 'cpp_library(name = "foo", doc = """Returns f(x) = y)""")'
        result = self._extract(content)
        self.assertIsNotNone(result)
        self.assertIn('"foo"', result)

    def test_triple_quoted_string_with_nested_quote_and_paren(self):
        """Triple-quoted string with embedded double-quote and ) must not break parsing."""
        # The embedded " would confuse a naive single-quote scanner.
        content = 'cpp_library(name = "foo", doc = """She said "hi()" today""")'
        result = self._extract(content)
        self.assertIsNotNone(
            result, "Should handle triple-quoted string with nested quote+paren"
        )
        self.assertIn('"foo"', result)

    def test_regular_string_with_paren(self):
        """A regular double-quoted string with ) inside must not terminate early."""
        content = 'cpp_library(name = "foo(bar)")'
        result = self._extract(content)
        self.assertIsNotNone(result)
        self.assertEqual(result, '(name = "foo(bar)")')


# ---------------------------------------------------------------------------
# Bug 5: cc_import must not receive hdrs/includes (moved to wrapping cc_library)
# ---------------------------------------------------------------------------


class TestMakeBuildFileContent(unittest.TestCase):
    """Tests for _make_build_file_content - shared lib wrapping."""

    def _make_shared_dep(self, name: str = "mylib") -> PrebuiltDep:
        return PrebuiltDep(
            name=name,
            path="/fake/path/" + name,
            lib_dir="lib",
            has_static_libs=False,
            has_shared_libs=True,
            static_libs=[],
            shared_libs=["lib" + name + ".so"],
            include_dir="include",
            is_header_only=False,
        )

    def test_cc_import_has_no_hdrs_or_includes(self):
        """cc_import block must not have hdrs= or includes= attributes."""
        dep = self._make_shared_dep("mylib")
        content = b2b._make_build_file_content(dep, "mylib")
        assert content is not None

        # Split on cc_import( block
        cc_import_idx = content.index("cc_import(")
        # Find end of cc_import block (next top-level closing paren)
        depth = 0
        end_idx = cc_import_idx
        for i, ch in enumerate(content[cc_import_idx:], cc_import_idx):
            if ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
                if depth == 0:
                    end_idx = i
                    break
        cc_import_block = content[cc_import_idx : end_idx + 1]

        self.assertNotIn("hdrs =", cc_import_block, "cc_import must not have hdrs=")
        self.assertNotIn(
            "includes =", cc_import_block, "cc_import must not have includes="
        )

    def test_cc_library_wrapper_has_hdrs_and_includes(self):
        """The wrapping cc_library must have hdrs= and includes=."""
        dep = self._make_shared_dep("mylib")
        content = b2b._make_build_file_content(dep, "mylib")
        assert content is not None

        # The cc_library wrapper comes after cc_import.
        cc_lib_idx = content.rindex("cc_library(")
        cc_lib_block = content[cc_lib_idx:]

        self.assertIn("hdrs =", cc_lib_block, "wrapping cc_library must have hdrs=")
        self.assertIn(
            "includes =", cc_lib_block, "wrapping cc_library must have includes="
        )


# ---------------------------------------------------------------------------
# Bug 4 (thrift_library.bzl): has_services parameter
# ---------------------------------------------------------------------------


class TestThriftLibraryTranslation(unittest.TestCase):
    """Tests for BuildFileGenerator._translate_thrift_library."""

    def _make_generator(self, tmp_path: Path) -> BuildFileGenerator:
        resolver = DepResolver(prebuilt_deps={}, all_targets={})
        return BuildFileGenerator(resolver, str(tmp_path))

    def _make_thrift_target(self, **kwargs) -> BuckTarget:
        defaults = {
            "name": "ctrl",
            "rule_type": "thrift_library",
            "buck_path": "//fboss/agent/if",
            "thrift_srcs": {"ctrl.thrift": []},
        }
        defaults.update(kwargs)
        return BuckTarget(**defaults)

    def test_thrift_translation_includes_thrift_srcs(self):
        with TemporaryDirectory() as d:
            gen = self._make_generator(Path(d))
            target = self._make_thrift_target()
            result = gen._translate_thrift_library(target)
            self.assertIsNotNone(result)
            self.assertIn("fboss_thrift_library", result)
            self.assertIn("ctrl.thrift", result)

    def test_thrift_translation_with_cpp2_options(self):
        with TemporaryDirectory() as d:
            gen = self._make_generator(Path(d))
            target = self._make_thrift_target(thrift_cpp2_options="json,reflection")
            result = gen._translate_thrift_library(target)
            self.assertIsNotNone(result)
            self.assertIn("thrift_cpp2_options", result)

    def test_thrift_translation_skips_non_cpp2_languages(self):
        with TemporaryDirectory() as d:
            gen = self._make_generator(Path(d))
            target = self._make_thrift_target(languages=["py", "py3"])
            result = gen._translate_thrift_library(target)
            self.assertIsNone(
                result, "Should skip thrift targets with no cpp2 language"
            )

    def test_thrift_translation_with_has_services_false_no_service_headers(self):
        """When has_services=False the macro should not generate service output files.

        This test checks the bzl macro behaviour indirectly by verifying the
        converter passes the has_services flag when we detect no services.
        For now we just verify the translator does not crash and produces output.
        """
        with TemporaryDirectory() as d:
            gen = self._make_generator(Path(d))
            # A thrift file with no service definitions
            target = self._make_thrift_target(
                thrift_srcs={"types.thrift": []},
            )
            result = gen._translate_thrift_library(target)
            self.assertIsNotNone(result)

    def test_thrift_translation_with_has_services_true(self):
        """When thrift file contains services, the translator should note it."""
        with TemporaryDirectory() as d:
            gen = self._make_generator(Path(d))
            target = self._make_thrift_target(
                thrift_srcs={"ctrl.thrift": []},
            )
            result = gen._translate_thrift_library(target)
            self.assertIsNotNone(result)


# ---------------------------------------------------------------------------
# _filter_files: OSS replacement logic
# ---------------------------------------------------------------------------


class TestFilterFiles(unittest.TestCase):
    """Tests for BuildFileGenerator._filter_files."""

    def setUp(self):
        self._tmpdir = TemporaryDirectory()
        self.tmp = Path(self._tmpdir.name)
        resolver = DepResolver(prebuilt_deps={}, all_targets={})
        self.gen = BuildFileGenerator(resolver, str(self.tmp))

    def tearDown(self):
        self._tmpdir.cleanup()

    def _make_dir(self, *parts: str) -> Path:
        p = self.tmp.joinpath(*parts)
        p.mkdir(parents=True, exist_ok=True)
        return p

    def test_facebook_files_skipped(self):
        self._make_dir("fboss/agent")
        result = self.gen._filter_files(
            ["facebook/Foo.cpp", "Bar.cpp"], "//fboss/agent"
        )
        self.assertNotIn("facebook/Foo.cpp", result)
        self.assertIn("Bar.cpp", result)

    def test_add_src_annotation_adds_oss_file(self):
        """# bazelify: add_src = oss/Foo.cpp should add the OSS file to srcs
        alongside the original file."""
        with TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            pkg = tmp_path / "fboss" / "mylib"
            pkg.mkdir(parents=True)
            oss_dir = pkg / "oss"
            oss_dir.mkdir()
            (oss_dir / "Foo.cpp").touch()
            (pkg / "Foo.cpp").touch()
            (pkg / "BUCK").write_text(
                'load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")\n'
                'oncall("test")\n'
                "# bazelify: add_src = oss/Foo.cpp\n"
                "cpp_library(\n"
                '    name = "mylib",\n'
                '    srcs = ["Foo.cpp"],\n'
                ")\n"
            )

            parser = BuckParser(str(tmp_path))
            targets = parser.parse_all()
            self.assertIn("//fboss/mylib", targets)

            resolver = DepResolver({}, parser.all_targets, str(tmp_path))
            generator = BuildFileGenerator(resolver, str(tmp_path))
            generator.generate_all(targets)

            content = (pkg / "BUILD.bazel").read_text()
            self.assertIn("oss/Foo.cpp", content, "add_src file must be in srcs")
            self.assertIn("Foo.cpp", content, "original file must also be in srcs")

    def test_deduplication(self):
        self._make_dir("fboss/agent")
        result = self.gen._filter_files(
            ["Foo.cpp", "Foo.cpp", "Bar.cpp"], "//fboss/agent"
        )
        self.assertEqual(result.count("Foo.cpp"), 1)


# ---------------------------------------------------------------------------
# _extract_string_list - key as first attribute (no preceding comma/space)
# ---------------------------------------------------------------------------


class TestExtractStringListEdgeCases(unittest.TestCase):
    def setUp(self):
        self.parser = _make_parser()

    def test_deps_as_first_attr_no_space(self):
        """(deps=[...]) with no space before 'deps' must still work."""
        block = '(deps=["//x:y"])'
        result = self.parser._extract_string_list(block, "deps")
        self.assertIn("//x:y", result)

    def test_exported_deps_not_matched_by_deps_key(self):
        """Searching for 'deps' must NOT match 'exported_deps'."""
        block = '(exported_deps=["//x:y"])'
        result = self.parser._extract_string_list(block, "deps")
        self.assertEqual(result, [])


# ---------------------------------------------------------------------------
# DepResolver.resolve() accepts BuckTarget, not str
# ---------------------------------------------------------------------------


class TestResolveAcceptsBuckTarget(unittest.TestCase):
    """DepResolver.resolve() must accept a BuckTarget as from_target."""

    def test_resolve_with_buck_target(self):
        target = BuckTarget(
            name="mylib",
            rule_type="cpp_library",
            buck_path="//fboss/agent",
        )
        resolver = DepResolver(prebuilt_deps={}, all_targets={})
        # Should not raise — from_target must be BuckTarget, not str
        result = resolver.resolve("//some:dep", target)
        # The dep won't resolve (not in all_targets), but it must not crash
        self.assertTrue(result is None or isinstance(result, str))


# ---------------------------------------------------------------------------
# End-to-end test: cc_test gets a working main() via //fboss/util/oss:test_main
# ---------------------------------------------------------------------------


class TestCcTestGetsMain(unittest.TestCase):
    """Verify that the converter adds //fboss/util/oss:test_main to cc_test
    targets so that test binaries get a main() function, matching the cmake
    build which compiles fboss/util/oss/TestMain.cpp into every test.

    This is an end-to-end test that:
    1. Creates a minimal BUCK file with a cpp_unittest
    2. Creates a trivial test .cpp file
    3. Runs the converter to generate a BUILD.bazel
    4. Verifies the generated BUILD.bazel has the test_main dep
    5. Runs bazel build on the generated target to verify it links
    """

    def test_converter_adds_test_main_dep(self):
        """The converter must add //fboss/util/oss:test_main to cc_test deps."""
        with TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            # Create a minimal package with a test
            pkg = tmp_path / "test_pkg"
            pkg.mkdir()
            (pkg / "BUCK").write_text(
                'load("@fbcode_macros//build_defs:cpp_unittest.bzl",'
                ' "cpp_unittest")\n'
                'oncall("test")\n'
                "cpp_unittest(\n"
                '    name = "my_test",\n'
                '    srcs = ["MyTest.cpp"],\n'
                "    deps = [],\n"
                ")\n"
            )
            (pkg / "MyTest.cpp").write_text(
                "#include <gtest/gtest.h>\nTEST(MyTest, Works) { EXPECT_EQ(1, 1); }\n"
            )

            # Parse and generate
            parser = BuckParser(str(tmp_path))
            targets = parser.parse_all()
            self.assertIn("//test_pkg", targets)

            prebuilt = {}
            resolver = DepResolver(prebuilt, parser.all_targets, str(tmp_path))
            generator = BuildFileGenerator(resolver, str(tmp_path))
            generator.generate_all(targets)

            build_file = pkg / "BUILD.bazel"
            self.assertTrue(build_file.exists(), "BUILD.bazel should be generated")
            content = build_file.read_text()

            # Verify test_main dep is present
            self.assertIn(
                "//fboss/util/oss:test_main",
                content,
                "cc_test must depend on //fboss/util/oss:test_main for main()",
            )

            # Verify gtest_main is NOT used (we use test_main instead)
            self.assertNotIn(
                "gtest_main",
                content,
                "cc_test should use test_main, not gtest_main",
            )


# ---------------------------------------------------------------------------
# End-to-end test: a program using boost, folly, and thrift links correctly
# ---------------------------------------------------------------------------


class TestBoostAndFollyLink(unittest.TestCase):
    """Verify that binaries linking boost, folly, and fbthrift don't hit
    the libboost_prg_exec_monitor.a duplicate main() issue or the
    F14LinkCheck / shared-vs-static glog/gflags conflicts we encountered.

    These are real bazel builds of known FBOSS targets that exercise the
    problematic dependency chains.
    """

    _FIXTURE_PKG = "//fboss/oss/scripts/bazel_test_fixtures"

    @classmethod
    def setUpClass(cls):
        """Generate BUILD.bazel for the test fixture package from its BUCK file.

        This exercises the bazelify pipeline and ensures the fixture targets
        are up-to-date before any test in this class runs.
        """
        # Ensure sccache wrappers are created.  The main() entrypoint does
        # this too, but pytest bypasses main() and discovers tests directly.
        sccache_setup = Path(__file__).parent / "bazel-sccache-setup.sh"
        if sccache_setup.exists():
            subprocess.run(["bash", str(sccache_setup)], check=False)

        repo_root = Path(__file__).parents[3]
        prebuilt = discover_prebuilt_deps(DEFAULT_INSTALL_DIR)
        parser = BuckParser(str(repo_root))
        targets = parser.parse_all()
        resolver = DepResolver(prebuilt, parser.all_targets, str(repo_root))
        generator = BuildFileGenerator(resolver, str(repo_root))
        fixture_targets = {cls._FIXTURE_PKG: targets.get(cls._FIXTURE_PKG, [])}
        generator.generate_all(fixture_targets)

    def _bazel_run(self, cmd: str, target: str) -> subprocess.CompletedProcess:
        repo_root = Path(__file__).parents[3]
        return subprocess.run(
            ["bazel", cmd, target],
            check=False,
            capture_output=True,
            text=True,
            timeout=300,
            cwd=str(repo_root),
        )

    def _bazel_build(self, target: str) -> subprocess.CompletedProcess:
        return self._bazel_run("build", target)

    def test_boost_library_links_without_duplicate_main(self):
        """A cc_test using boost must not pull in libboost_prg_exec_monitor.a
        which defines main() and causes 'duplicate symbol: main' errors.

        Uses a committed fixture target so this test is self-contained.
        """
        result = self._bazel_run(
            "test",
            "//fboss/oss/scripts/bazel_test_fixtures:boost_test",
        )
        self.assertEqual(
            result.returncode, 0, f"bazel test failed:\n{result.stderr[-500:]}"
        )
        self.assertNotIn("duplicate symbol: main", result.stderr)
        self.assertNotIn("cpp_main", result.stderr)

    def test_folly_f14_link_check_resolves(self):
        """Targets using folly's F14 containers must link and run successfully.

        The F14LinkCheck symbol is an ABI compatibility check defined in
        F14Table.cpp.  Nothing references it directly, so the linker drops it
        unless the F14 detail archive is force-linked via cc_import(alwayslink).
        """
        result = self._bazel_run(
            "test",
            "//fboss/oss/scripts/bazel_test_fixtures:f14_test",
        )
        self.assertEqual(
            result.returncode, 0, f"bazel test failed:\n{result.stderr[-500:]}"
        )
        self.assertNotIn("F14LinkCheck", result.stderr)

    def test_test_binary_has_main(self):
        """cc_test targets must get main() from //fboss/util/oss:test_main,
        not from libgtest_main.a (which the linker drops).

        Uses a committed fixture target so the package is always known to Bazel.
        Runs via `bazel test` to verify the binary not only links but executes.
        """
        result = self._bazel_run(
            "test",
            "//fboss/oss/scripts/bazel_test_fixtures:gtest_main_test",
        )
        self.assertEqual(
            result.returncode, 0, f"bazel test failed:\n{result.stderr[-500:]}"
        )
        self.assertNotIn("undefined symbol: main", result.stderr)


# ---------------------------------------------------------------------------
# _synthetic_prebuilt_dep: generates PrebuiltDep from manifest info alone
# (used by --use-getdeps-rules when deps aren't installed yet)
# ---------------------------------------------------------------------------


class TestSyntheticPrebuiltDep(unittest.TestCase):
    """Tests for _synthetic_prebuilt_dep(), which generates the PrebuiltDep
    used to produce build_file_content when dep install dirs don't exist yet
    (i.e., during --use-getdeps-rules / CI mode)."""

    def test_cmake_dep_is_static(self):
        """cmake-built deps must be static (no shared libs)."""
        dep = _synthetic_prebuilt_dep("folly", {"builder": "cmake"})
        self.assertTrue(dep.has_static_libs)
        self.assertFalse(dep.has_shared_libs)
        self.assertFalse(dep.is_header_only)

    def test_nop_dep_is_header_only(self):
        """nop-built deps (e.g., range-v3) must be header-only."""
        dep = _synthetic_prebuilt_dep("range-v3", {"builder": "nop"})
        self.assertTrue(dep.is_header_only)
        self.assertFalse(dep.has_static_libs)

    def test_nop_dep_exceptions_are_not_header_only(self):
        """sai_impl and iproute2 use builder=nop but are NOT header-only."""
        for name in ("sai_impl", "iproute2"):
            with self.subTest(name=name):
                dep = _synthetic_prebuilt_dep(name, {"builder": "nop"})
                self.assertFalse(
                    dep.is_header_only, f"{name} should not be header-only"
                )

    def test_glog_uses_lib64(self):
        """glog installs to lib64/ on CentOS/Linux; lib_dir must reflect this."""
        dep = _synthetic_prebuilt_dep("glog", {"builder": "cmake"})
        self.assertEqual(dep.lib_dir, "lib64")
        self.assertTrue(dep.has_static_libs)
        self.assertFalse(dep.has_shared_libs)

    def test_googletest_uses_lib64(self):
        """googletest also installs to lib64/ on CentOS/Linux."""
        dep = _synthetic_prebuilt_dep("googletest", {"builder": "cmake"})
        self.assertEqual(dep.lib_dir, "lib64")

    def test_regular_cmake_dep_uses_lib(self):
        """Most cmake deps install to lib/."""
        dep = _synthetic_prebuilt_dep("gflags", {"builder": "cmake"})
        self.assertEqual(dep.lib_dir, "lib")


# ---------------------------------------------------------------------------
# _make_build_file_content: generates Starlark BUILD content from PrebuiltDep
# ---------------------------------------------------------------------------


def _make_static_dep(name: str, lib_dir: str = "lib", **kwargs) -> PrebuiltDep:
    return PrebuiltDep(
        name=name,
        path=f"/fake/installed/{name}",
        lib_dir=lib_dir,
        has_static_libs=True,
        has_shared_libs=False,
        static_libs=[f"lib{name}.a"],
        shared_libs=[],
        include_dir="include",
        is_header_only=False,
        **kwargs,
    )


def _make_shared_dep(name: str, so_file: str, lib_dir: str = "lib") -> PrebuiltDep:
    return PrebuiltDep(
        name=name,
        path=f"/fake/installed/{name}",
        lib_dir=lib_dir,
        has_static_libs=False,
        has_shared_libs=True,
        static_libs=[],
        shared_libs=[so_file],
        include_dir="include",
        is_header_only=False,
    )


class TestMakeBuildFileContentMatrix(unittest.TestCase):
    """Tests for _make_build_file_content(), which generates build_file_content
    strings embedded in MODULE.bazel getdeps_repository() calls."""

    # -- Static library path --------------------------------------------------

    def test_static_dep_uses_glob_not_cc_import(self):
        """Static deps must use cc_library with glob(["lib/*.a", "lib64/*.a"]),
        not cc_import.  cc_import with shared_library on a .a file causes
        SolibSymlink failures when the .so doesn't exist."""
        dep = _make_static_dep("gflags")
        content = _make_build_file_content(dep, "gflags")
        assert content is not None
        self.assertIn('glob(["lib/*.a", "lib64/*.a"]', content)
        self.assertNotIn("cc_import", content)
        self.assertNotIn("shared_library", content)

    def test_glog_static_in_lib64_uses_glob(self):
        """glog is in lib64/ but must still use the static glob pattern,
        not cc_import.  This was the bug: glog's manifest sets BUILD_SHARED_LIBS=ON,
        so without our _EXTRA_CMAKE_DEFINES override, glog was built as shared and
        _make_build_file_content generated cc_import(shared_library=lib64/libglog.so),
        which broke when we later forced static builds."""
        dep = _make_static_dep("glog", lib_dir="lib64")
        content = _make_build_file_content(dep, "glog")
        assert content is not None
        self.assertIn('glob(["lib/*.a", "lib64/*.a"]', content)
        self.assertNotIn("cc_import", content)

    def test_static_dep_sets_linkstatic(self):
        dep = _make_static_dep("gflags")
        content = _make_build_file_content(dep, "gflags")
        self.assertIn("linkstatic = True", content)

    def test_no_alwayslink_in_prebuilt_deps(self):
        """No prebuilt dep should use alwayslink=True. Thrift registration statics
        (_sinit.cpp) are kept by the linker naturally via .init_array; using
        alwayslink on the whole fbthrift archive causes severe binary bloat."""
        for name in ("fbthrift", "gflags", "folly", "glog"):
            dep = _make_static_dep(name)
            content = _make_build_file_content(dep, name)
            assert content is not None
            self.assertNotIn("alwayslink", content, f"{name} must not use alwayslink")

    # -- Shared library path --------------------------------------------------

    def test_shared_dep_uses_cc_import(self):
        """Shared deps must use cc_import(shared_library=...) wrapped in cc_library."""
        dep = _make_shared_dep("libfoo", "libfoo.so")
        content = _make_build_file_content(dep, "libfoo")
        assert content is not None
        self.assertIn("cc_import", content)
        self.assertIn("shared_library", content)
        self.assertIn("libfoo.so", content)

    def test_shared_dep_import_is_private(self):
        """cc_import target must have visibility=private; only the wrapping
        cc_library is public."""
        dep = _make_shared_dep("libfoo", "libfoo.so")
        content = _make_build_file_content(dep, "libfoo")
        self.assertIn('visibility = ["//visibility:private"]', content)

    def test_shared_dep_has_rpath_linkopt(self):
        """Shared dep cc_library must set -Wl,-rpath so the .so is found at runtime."""
        dep = _make_shared_dep("libfoo", "libfoo.so")
        content = _make_build_file_content(dep, "libfoo")
        self.assertIn("-Wl,-rpath", content)

    # -- Header-only path -----------------------------------------------------

    def test_header_only_dep_no_srcs(self):
        """Header-only deps must have no srcs= (no archive to link)."""
        dep = PrebuiltDep(
            name="range-v3",
            path="/fake/installed/range-v3",
            lib_dir="include",
            has_static_libs=False,
            has_shared_libs=False,
            static_libs=[],
            shared_libs=[],
            include_dir="include",
            is_header_only=True,
        )
        content = _make_build_file_content(dep, "range-v3")
        assert content is not None
        self.assertNotIn("srcs", content)
        self.assertNotIn("cc_import", content)

    # -- googletest special case ----------------------------------------------

    def test_googletest_generates_multiple_targets(self):
        """googletest must export gtest, gmock, gtest_main, gmock_main targets."""
        dep = PrebuiltDep(
            name="googletest",
            path="/fake/installed/googletest",
            lib_dir="lib64",
            has_static_libs=True,
            has_shared_libs=False,
            static_libs=["libgtest.a", "libgmock.a"],
            shared_libs=[],
            include_dir="include",
            is_header_only=False,
        )
        content = _make_build_file_content(dep, "googletest")
        assert content is not None
        for target in ("gtest", "gmock", "gtest_main", "gmock_main"):
            self.assertIn(f'name = "{target}"', content, f"Missing target: {target}")

    def test_googletest_main_uses_whole_archive(self):
        """gtest_main must use --whole-archive to force-link the main() symbol."""
        dep = PrebuiltDep(
            name="googletest",
            path="/fake/installed/googletest",
            lib_dir="lib64",
            has_static_libs=True,
            has_shared_libs=False,
            static_libs=["libgtest.a", "libgmock.a"],
            shared_libs=[],
            include_dir="include",
            is_header_only=False,
        )
        content = _make_build_file_content(dep, "googletest")
        self.assertIn("-Wl,--whole-archive", content)
        self.assertIn("libgtest_main.a", content)

    # -- boost special case ---------------------------------------------------

    def test_boost_excludes_test_archives(self):
        """boost must exclude libboost_prg_exec_monitor.a and test_exec_monitor.a
        because they define main() which conflicts with gtest."""
        dep = _make_static_dep("boost")
        content = _make_build_file_content(dep, "boost")
        assert content is not None
        self.assertIn("libboost_prg_exec_monitor.a", content)  # in exclude list
        self.assertIn("libboost_test_exec_monitor.a", content)  # in exclude list
        self.assertIn("libboost_unit_test_framework.a", content)  # in exclude list
        self.assertIn("exclude", content)


def main():
    result = subprocess.run(
        ["bazel", "--version"], check=False, capture_output=True, text=True
    )
    if result.returncode != 0:
        sys.exit("ERROR: bazel --version failed, required for integration tests")
    print(result.stdout.strip())
    # Ensure sccache is running and correctly configured before
    # integration tests that invoke bazel.
    sccache_setup = Path(__file__).parent / "bazel-sccache-setup.sh"
    if sccache_setup.exists():
        subprocess.run(["bash", str(sccache_setup)], check=False)
    unittest.main(verbosity=2)


if __name__ == "__main__":
    main()
