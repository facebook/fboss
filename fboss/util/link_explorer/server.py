#!/usr/bin/env python3

# pyre-unsafe
"""
Link Explorer — Flask web server for FBOSS switch topology exploration.

Runs on Tupperware with Gunicorn, or locally on a devserver.
Health check at /health returns "__OK__" for TW monitoring.
"""

import argparse
import os
import re

from fboss.util.link_explorer.link_explorer_app import query_switch
from flask import Flask, jsonify, render_template, request


app = Flask(__name__, template_folder="templates")


# ── Health Check ─────────────────────────────────────────────────────────────


@app.route("/health")
def health():
    return "__OK__"


# ── Routes ───────────────────────────────────────────────────────────────────


@app.route("/")
def index():
    return render_template("index.html")


@app.route("/api/query")
def api_query():
    hostname = request.args.get("hostname", "").strip()
    if not hostname:
        return jsonify({"error": "No hostname provided"})
    if not re.match(r"^[a-zA-Z0-9._-]+$", hostname):
        return jsonify({"error": "Invalid hostname"})
    data = query_switch(hostname)
    return jsonify(data)


# ── Main ─────────────────────────────────────────────────────────────────────


def main():
    parser = argparse.ArgumentParser(description="FBOSS Link Explorer")
    parser.add_argument("--port", type=int, default=int(os.environ.get("PORT", 8080)))
    parser.add_argument("--debug", type=str, default=os.environ.get("DEBUG", "false"))
    args = parser.parse_args()

    port = args.port
    debug = args.debug.lower() == "true"

    print(f"[Link Explorer] Starting on port {port}")

    app.run(
        host="::",
        port=port,
        debug=debug,
        use_reloader=False,
    )


if __name__ == "__main__":
    main()
