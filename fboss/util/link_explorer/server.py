#!/usr/bin/env python3
"""
Link Explorer — Flask web server for FBOSS switch topology exploration.

Runs on Tupperware with Gunicorn, or locally on a devserver.
Health check at /health returns "__OK__" for TW monitoring.
"""

import argparse
import os
import re
import socket
import ssl

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


# ── SSL Context ──────────────────────────────────────────────────────────────

TW_CERT = "/var/facebook/tupperware/tls/x509_identities/server.pem"
DEV_CERT = "/var/facebook/x509_identities/server.pem"


def get_ssl_context():
    """Get SSL context for Tupperware deployment."""
    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    ctx.load_cert_chain(TW_CERT, TW_CERT)
    return ctx


def get_dev_ssl_context():
    """Get SSL context for devserver."""
    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    ctx.load_cert_chain(DEV_CERT, DEV_CERT)
    return ctx


def is_dev_server():
    """Detect if running on a devserver (not Tupperware)."""
    hostname = socket.gethostname()
    return bool(re.match(r"dev", hostname))


# ── Main ─────────────────────────────────────────────────────────────────────


def main():
    parser = argparse.ArgumentParser(description="FBOSS Link Explorer")
    parser.add_argument("--port", type=int, default=int(os.environ.get("PORT", 8080)))
    parser.add_argument("--debug", type=str, default=os.environ.get("DEBUG", "false"))
    args = parser.parse_args()

    port = args.port
    debug = args.debug.lower() == "true"

    if is_dev_server():
        ssl_ctx = get_dev_ssl_context()
        print(f"[Link Explorer] Dev mode on port {port}")
    else:
        ssl_ctx = get_ssl_context()
        print(f"[Link Explorer] Production mode on port {port}")

    app.run(
        host="::",
        port=port,
        ssl_context=ssl_ctx,
        debug=debug,
        use_reloader=False,
    )


if __name__ == "__main__":
    main()
