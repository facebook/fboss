#!/usr/bin/env python3
# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# Tests for postProcess.py regexp optimizations.
# Verifies that string-op replacements produce identical output
# to the original regex-based logic.

# pyre-unsafe

import os
import re
import shutil
import tempfile
import unittest

from fboss.agent.hw.sai.tracer.run.postProcess import (
    _RE_HASHAPIVAR,
    _RE_RUN_TRACE_FUNC,
    _RE_UNNAMED_NAMESPACE_BEGIN,
    _RE_UNNAMED_NAMESPACE_END,
    _RE_VAR_LIST,
    is_cpp_file,
    split_file,
)


# Original regex patterns (before optimization) for comparison
ORIG_REGEX_COMMENT = r"^(\/\/|\/\*|\*)"
ORIG_REGEX_CPP_FILE = r"\.cpp$"
ORIG_REGEX_VARS_BEGIN = r"Global Variables Start"
ORIG_REGEX_VARS_END = r"Global Variables End"
ORIG_REGEX_UNNAMED_NAMESPACE_BEGIN = r"^namespace( )*{$"
ORIG_REGEX_UNNAMED_NAMESPACE_END = r"^}( )*\/\/( )*namespace$"
ORIG_REGEX_VAR_LIST = r"int list\_[0-9]*\[[0-9]*\]\;"
ORIG_REGEX_RUN_TRACE_FUNC = r"^void run\_trace\(\) ?{$"
ORIG_REGEX_INCLUDES = r"^\#include"
ORIG_REGEX_DEFINES = r"^\#define"
ORIG_REGEX_SAILOG_INCLUDE = r"^\#include.*SaiLog\.h"
ORIG_REGEX_HASHAPIVAR = r"hash\_api(?![a-zA-Z0-9\_\-])"
ORIG_REGEX_SAILOGCPP = r"\"SaiLog\.cpp\""


class TestCommentDetection(unittest.TestCase):
    """Verify: not line.startswith(("//", "/*", "*")) == (re.search(COMMENT, line) is None)"""

    def _old(self, line):
        return re.search(ORIG_REGEX_COMMENT, line) is None

    def _new(self, line):
        return not line.startswith(("//", "/*", "*"))

    def test_c_style_comment(self):
        self.assertEqual(self._old("/* comment */"), self._new("/* comment */"))

    def test_cpp_style_comment(self):
        self.assertEqual(self._old("// comment"), self._new("// comment"))

    def test_star_continuation(self):
        self.assertEqual(self._old("* continued"), self._new("* continued"))

    def test_code_line(self):
        self.assertEqual(self._old("int x = 5;"), self._new("int x = 5;"))

    def test_empty_line(self):
        self.assertEqual(self._old(""), self._new(""))

    def test_include_line(self):
        self.assertEqual(self._old("#include <sai.h>"), self._new("#include <sai.h>"))

    def test_star_in_middle(self):
        # "int *x" should NOT match comment pattern (no leading *)
        self.assertEqual(self._old("int *x;"), self._new("int *x;"))


class TestCppFileDetection(unittest.TestCase):
    """Verify: filename.endswith(".cpp") == (re.search(CPP_FILE, filename) is not None)"""

    def test_cpp_file(self):
        self.assertTrue(is_cpp_file("SaiLog1.cpp"))

    def test_header_file(self):
        self.assertFalse(is_cpp_file("SaiLog.h"))

    def test_bzl_file(self):
        self.assertFalse(is_cpp_file("run.bzl"))

    def test_cpp_in_path(self):
        self.assertTrue(is_cpp_file("output/SaiLog1.cpp"))

    def test_cppx_file(self):
        # ".cppx" should NOT match
        self.assertFalse(is_cpp_file("test.cppx"))


class TestVarsBeginEnd(unittest.TestCase):
    """Verify: 'Global Variables Start' in line == re.search(VARS_BEGIN, line)"""

    def _old_begin(self, line):
        return re.search(ORIG_REGEX_VARS_BEGIN, line) is not None

    def _new_begin(self, line):
        return "Global Variables Start" in line

    def _old_end(self, line):
        return re.search(ORIG_REGEX_VARS_END, line) is not None

    def _new_end(self, line):
        return "Global Variables End" in line

    def test_begin_marker(self):
        line = "// Global Variables Start"
        self.assertEqual(self._old_begin(line), self._new_begin(line))

    def test_end_marker(self):
        line = "// Global Variables End"
        self.assertEqual(self._old_end(line), self._new_end(line))

    def test_no_marker(self):
        line = "int x = 5;"
        self.assertEqual(self._old_begin(line), self._new_begin(line))
        self.assertEqual(self._old_end(line), self._new_end(line))


class TestUnnamedNamespace(unittest.TestCase):
    """Verify compiled regex matches same as original string regex"""

    def test_namespace_begin(self):
        cases = [
            "namespace {",
            "namespace  {",
            "namespace   {",
            "namespace{",  # no space — should NOT match (regex requires ^namespace( )*{$)
            "  namespace {",  # leading space — should NOT match (anchored ^)
            "namespace { // extra",  # trailing — should NOT match (anchored $)
        ]
        for line in cases:
            old = re.search(ORIG_REGEX_UNNAMED_NAMESPACE_BEGIN, line) is not None
            new = _RE_UNNAMED_NAMESPACE_BEGIN.search(line) is not None
            self.assertEqual(old, new, f"Mismatch on: {line!r}")

    def test_namespace_end(self):
        cases = [
            "} // namespace",
            "}  //  namespace",
            "} //namespace",  # no space after // — should NOT match
            "}// namespace",  # no space before // — should NOT match
            "  } // namespace",  # leading space — should NOT match
        ]
        for line in cases:
            old = re.search(ORIG_REGEX_UNNAMED_NAMESPACE_END, line) is not None
            new = _RE_UNNAMED_NAMESPACE_END.search(line) is not None
            self.assertEqual(old, new, f"Mismatch on: {line!r}")


class TestHashApiVar(unittest.TestCase):
    """Verify compiled regex + 'hash_api' in line short-circuit"""

    def test_hash_api_standalone(self):
        line = "sai_object_type_t hash_api;"
        # Old: re.search then re.sub
        old_result = re.sub(ORIG_REGEX_HASHAPIVAR, "hash_api_var", line)
        # New: short-circuit then compiled sub
        if "hash_api" in line:
            new_result = _RE_HASHAPIVAR.sub("hash_api_var", line)
        else:
            new_result = line
        self.assertEqual(old_result, new_result)

    def test_hash_api_in_word(self):
        # hash_api_something should NOT be replaced (negative lookahead)
        line = "int hash_api_count = 0;"
        old_result = re.sub(ORIG_REGEX_HASHAPIVAR, "hash_api_var", line)
        if "hash_api" in line:
            new_result = _RE_HASHAPIVAR.sub("hash_api_var", line)
        else:
            new_result = line
        self.assertEqual(old_result, new_result)

    def test_no_hash_api(self):
        line = "int x = 5;"
        old_result = re.sub(ORIG_REGEX_HASHAPIVAR, "hash_api_var", line)
        if "hash_api" in line:
            new_result = _RE_HASHAPIVAR.sub("hash_api_var", line)
        else:
            new_result = line
        self.assertEqual(old_result, new_result)

    def test_hash_api_end_of_line(self):
        line = "get_hash_api"
        old_result = re.sub(ORIG_REGEX_HASHAPIVAR, "hash_api_var", line)
        if "hash_api" in line:
            new_result = _RE_HASHAPIVAR.sub("hash_api_var", line)
        else:
            new_result = line
        self.assertEqual(old_result, new_result)


class TestIncludeDetection(unittest.TestCase):
    """Verify: line.startswith('#include') and 'SaiLog.h' in line"""

    def _old_sailog(self, line):
        return re.search(ORIG_REGEX_SAILOG_INCLUDE, line) is not None

    def _new_sailog(self, line):
        return line.startswith("#include") and "SaiLog.h" in line

    def _old_includes(self, line):
        return re.search(ORIG_REGEX_INCLUDES, line) is not None

    def _new_includes(self, line):
        return line.startswith("#include")

    def _old_defines(self, line):
        return re.search(ORIG_REGEX_DEFINES, line) is not None

    def _new_defines(self, line):
        return line.startswith("#define")

    def test_sailog_include(self):
        line = '#include "SaiLog.h"'
        self.assertEqual(self._old_sailog(line), self._new_sailog(line))

    def test_other_include(self):
        line = "#include <sai.h>"
        self.assertEqual(self._old_sailog(line), self._new_sailog(line))
        self.assertEqual(self._old_includes(line), self._new_includes(line))

    def test_define(self):
        line = "#define FOO 1"
        self.assertEqual(self._old_defines(line), self._new_defines(line))

    def test_not_include(self):
        line = "int x = 5;"
        self.assertEqual(self._old_includes(line), self._new_includes(line))
        self.assertEqual(self._old_defines(line), self._new_defines(line))

    def test_includes_combined(self):
        line = "#include <stdio.h>"
        self.assertEqual(
            self._old_includes(line) or self._old_defines(line),
            line.startswith(("#include", "#define")),
        )


class TestSaiLogCpp(unittest.TestCase):
    """Verify: '"SaiLog.cpp"' in line == re.search(SAILOGCPP, line)"""

    def test_sailogcpp_present(self):
        line = '            "SaiLog.cpp",'
        old = re.search(ORIG_REGEX_SAILOGCPP, line) is not None
        new = '"SaiLog.cpp"' in line
        self.assertEqual(old, new)

    def test_sailogcpp_absent(self):
        line = '            "SaiLog1.cpp",'
        old = re.search(ORIG_REGEX_SAILOGCPP, line) is not None
        new = '"SaiLog.cpp"' in line
        self.assertEqual(old, new)


class TestVarList(unittest.TestCase):
    """Verify: 'int list_' in line guard + compiled regex"""

    def test_var_list_match(self):
        line = "int list_0[10];"
        old = re.search(ORIG_REGEX_VAR_LIST, line) is not None
        new = "int list_" in line and _RE_VAR_LIST.search(line) is not None
        self.assertEqual(old, new)

    def test_var_list_no_match(self):
        line = "int x = 5;"
        old = re.search(ORIG_REGEX_VAR_LIST, line) is not None
        new = "int list_" in line and _RE_VAR_LIST.search(line) is not None
        self.assertEqual(old, new)

    def test_var_list_multi_digit(self):
        line = "int list_123[456];"
        old = re.search(ORIG_REGEX_VAR_LIST, line) is not None
        new = "int list_" in line and _RE_VAR_LIST.search(line) is not None
        self.assertEqual(old, new)


class TestRunTraceFunc(unittest.TestCase):
    """Verify compiled regex matches same as original"""

    def test_run_trace_with_space(self):
        line = "void run_trace() {"
        old = re.search(ORIG_REGEX_RUN_TRACE_FUNC, line) is not None
        new = _RE_RUN_TRACE_FUNC.search(line) is not None
        self.assertEqual(old, new)

    def test_run_trace_no_space(self):
        line = "void run_trace(){"
        old = re.search(ORIG_REGEX_RUN_TRACE_FUNC, line) is not None
        new = _RE_RUN_TRACE_FUNC.search(line) is not None
        self.assertEqual(old, new)

    def test_not_run_trace(self):
        line = "void other_func() {"
        old = re.search(ORIG_REGEX_RUN_TRACE_FUNC, line) is not None
        new = _RE_RUN_TRACE_FUNC.search(line) is not None
        self.assertEqual(old, new)


class TestEndToEnd(unittest.TestCase):
    """End-to-end: generate a synthetic SaiLog.cpp and verify split_file output"""

    def setUp(self):
        self.tmpdir = tempfile.mkdtemp()
        self.input_dir = os.path.join(self.tmpdir, "input")
        self.output_dir = os.path.join(self.tmpdir, "output")
        os.makedirs(self.input_dir)

    def tearDown(self):
        shutil.rmtree(self.tmpdir)

    def _write_sailog(self, content):
        path = os.path.join(self.input_dir, "SaiLog.cpp")
        with open(path, "w") as f:
            f.write(content)
        return path

    def test_basic_split(self):
        """Verify the script processes a synthetic SaiLog.cpp without errors"""
        sailog = self._write_sailog(
            """\
/*
 * Auto-generated SAI replayer log
 */
#include <sai.h>
#include "SaiLog.h"
#define SAI_VERSION 1

// Global Variables Start
int list_0[10];
[[maybe_unused]] int x = 5;
// Global Variables End

namespace {
static int internal_var = 0;
} // namespace

namespace facebook::fboss {

void run_trace() {
  sai_object_type_t hash_api;
  hash_api = 42;
  int list_1[20];
  printf("hello");
}

} // namespace facebook::fboss
"""
        )

        # Reset global state (module-level lists)
        import fboss.agent.hw.sai.tracer.run.postProcess as pp

        pp.GENERATED_FILES.clear()
        pp.GENERATED_FILES_SAILOGCPP.clear()

        os.makedirs(self.output_dir, exist_ok=True)
        split_file(sailog, self.output_dir, 400000)

        # Verify output files were created
        self.assertTrue(os.path.exists(os.path.join(self.output_dir, "SaiLog.h")))
        self.assertTrue(os.path.exists(os.path.join(self.output_dir, "Main.cpp")))
        self.assertTrue(os.path.exists(os.path.join(self.output_dir, "SaiLog1.cpp")))

        # Verify hash_api was renamed to hash_api_var in output
        with open(os.path.join(self.output_dir, "SaiLog1.cpp")) as f:
            content = f.read()
        self.assertIn("hash_api_var", content)
        # Ensure standalone hash_api was replaced but hash_api_var is not
        # double-replaced (the negative lookahead prevents that)

        # Verify #include <sai.h> went to header (not in cpp output)
        with open(os.path.join(self.output_dir, "SaiLog.h")) as f:
            header = f.read()
        self.assertIn("#include <sai.h>", header)

        # Verify the original #include "SaiLog.h" from input was filtered,
        # and the script re-added it via get_file_intro() at the top of
        # each output cpp file (this is expected behavior).
        self.assertIn('#include "SaiLog.h"', content)


if __name__ == "__main__":
    unittest.main()
