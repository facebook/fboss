"""Repository rule that builds a third-party dependency using getdeps with caching.

Each dependency is built by cached-get-deps.py, which either downloads a cached
tarball from a remote HTTP cache or builds from source using Meta's getdeps.py.

The dependency's manifest file (and optional revision/patch files) are declared
as inputs so Bazel re-executes the rule when they change.
"""

def _getdeps_repository_impl(ctx):
    # Touch declared inputs so Bazel tracks them for change detection.
    if ctx.attr.manifest:
        ctx.path(ctx.attr.manifest)
    if ctx.attr.revision_file:
        ctx.path(ctx.attr.revision_file)
    for patch in ctx.attr.patches:
        ctx.path(patch)

    # Resolve dependency install paths (forces correct build order).
    for dep_name in ctx.attr.dep_repos:
        ctx.path(Label("@" + dep_name + "//:BUILD"))

    # Locate scripts relative to the repo root.
    repo_root = str(ctx.path(Label("@//:MODULE.bazel")).dirname)
    build_script = repo_root + "/fboss/oss/scripts/cached-get-deps.py"
    scratch = ctx.attr.scratch_path
    name = ctx.attr.project_name

    # Compute project hash to determine expected install dir.
    hash_result = ctx.execute([
        "python3", build_script,
        "--project", name,
        "--scratch-path", scratch,
        "--hash-only",
    ], quiet = True)
    project_hash = hash_result.stdout.strip()

    # Determine expected install directory.
    if ctx.attr.first_party:
        expected_dir = scratch + "/installed/" + name
    else:
        expected_dir = scratch + "/installed/" + name + "-" + project_hash

    # Check if the expected dir exists.
    exists = ctx.execute(["test", "-d", expected_dir]).return_code == 0

    if not exists:
        # No existing install — build or fetch from cache.
        cmd = [
            "python3", build_script,
            "--project", name,
            "--scratch-path", scratch,
        ]
        if ctx.attr.cache_url:
            cmd += ["--cache-url", ctx.attr.cache_url]

        result = ctx.execute(cmd, timeout = 3600, quiet = False)
        if result.return_code != 0:
            fail("cached-get-deps.py failed for %s:\n%s\n%s" % (
                name, result.stdout, result.stderr,
            ))

        # Re-check for the install dir after building.
        exists = ctx.execute(["test", "-d", expected_dir]).return_code == 0
        if not exists:
            fail("Install directory not found for %s after build: %s" % (name, expected_dir))

    # Symlink installed directory contents into the repo root.
    entries_result = ctx.execute(["ls", expected_dir], quiet = True)
    if entries_result.return_code != 0:
        fail("Cannot list install dir: %s" % expected_dir)

    for entry in entries_result.stdout.strip().split("\n"):
        entry = entry.strip()
        if not entry or entry == ".built-by-getdeps":
            continue
        ctx.symlink(expected_dir + "/" + entry, entry)

    # Write the BUILD file.
    ctx.file("BUILD", ctx.attr.build_file_content)

getdeps_repository = repository_rule(
    implementation = _getdeps_repository_impl,
    attrs = {
        "project_name": attr.string(mandatory = True),
        "manifest": attr.label(allow_single_file = True),
        "revision_file": attr.label(allow_single_file = True),
        "patches": attr.label_list(default = []),
        "dep_repos": attr.string_list(default = []),
        "build_file_content": attr.string(mandatory = True),
        "first_party": attr.bool(default = False),
        "cache_url": attr.string(default = ""),
        "scratch_path": attr.string(default = "/var/FBOSS/tmp_bld_dir"),
    },
    local = True,
    environ = ["CC", "CXX", "CXXFLAGS", "CPPFLAGS", "LDFLAGS"],
)
