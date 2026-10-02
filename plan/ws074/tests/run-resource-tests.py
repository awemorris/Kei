#!/usr/bin/env python3
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Exercise native resource metadata against real local files and loopback HTTP."""

import argparse
import http.server
import os
from pathlib import Path
import subprocess
import tempfile
import threading


class ResourceHandler(http.server.BaseHTTPRequestHandler):
    def log_message(self, _format, *args):
        pass

    def do_GET(self):
        if self.path == "/entry.bin":
            self.send_response(302)
            self.send_header("Location", "/odd.html")
            self.send_header("Content-Length", "0")
            self.end_headers()
            return
        routes = {
            "/odd.html": (200, "IMAGE/SVG+XML; charset=utf-8", b"<r/>"),
            "/plain.svg": (200, "text/xml", b"<html/>"),
            "/missing.xml": (404, "text/html", b"missing"),
        }
        status, mime, body = routes.get(self.path, (404, "text/plain", b"absent"))
        self.send_response(status)
        self.send_header("Content-Type", mime)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Connection", "close")
        self.end_headers()
        self.wfile.write(body)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--program", required=True)
    args = parser.parse_args()
    server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), ResourceHandler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        with tempfile.TemporaryDirectory(prefix="ws074-resource-") as directory:
            for name in ["tree.xml", "tree.SVG", "tree.xhtml", "tree.htm", "tree.bin", "tree space.xml"]:
                Path(directory, name).write_bytes(b"<r/>")
            url = f"http://127.0.0.1:{server.server_port}/base.html"
            env = dict(os.environ, ASAN_OPTIONS="detect_stack_use_after_return=0")
            result = subprocess.run([args.program, directory, url], env=env,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    text=True, timeout=60)
            print(result.stdout, end="")
            return result.returncode
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=5)


if __name__ == "__main__":
    raise SystemExit(main())
