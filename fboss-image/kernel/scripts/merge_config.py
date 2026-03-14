#!/usr/bin/env python3
#
# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory
#

import pathlib
import re
import sys


def main(argv):
    if len(argv) != 3:
        sys.stderr.write("Usage: merge_config.py <config_path> <overlay_path>\n")
        return 2
    cfg_path, overlay_path = argv[1], argv[2]
    cfg_lines = pathlib.Path(cfg_path).read_text().splitlines()
    overlay_lines = pathlib.Path(overlay_path).read_text().splitlines()

    VAL_RE = re.compile(r"^CONFIG_([^=]+)=(.*)$")
    NS_RE = re.compile(r"^#\s*CONFIG_([^\s]+)\s+is\s+not\s+set\s*$")

    # Build updates from overlay, keeping only the last occurrence per key
    updates = {}
    order = []
    for raw in overlay_lines:
        line = raw.strip()
        if not line:
            continue
        # Skip pure comments except the canonical "not set" form
        if line.startswith("#") and not NS_RE.match(line):
            continue
        m_val = VAL_RE.match(line)
        m_ns = None if m_val else NS_RE.match(line)
        if not (m_val or m_ns):
            continue
        key = (m_val or m_ns).group(1)
        repl = (
            f"CONFIG_{key}={m_val.group(2)}" if m_val else f"# CONFIG_{key} is not set"
        )
        if key in updates:
            try:
                order.remove(key)
            except ValueError:
                pass
        updates[key] = repl
        order.append(key)

    # Filter out any existing entries for keys we will update
    out = []
    for l in cfg_lines:
        m = VAL_RE.match(l)
        if not m:
            m = NS_RE.match(l)
        k = m.group(1) if m else None
        if k is None or k not in updates:
            out.append(l)

    # Append replacements in overlay order
    for k in order:
        out.append(updates[k])

    pathlib.Path(cfg_path).write_text("\n".join(out) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
