#!/usr/bin/env python3
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Test actual child resource loading with ordinary delayed loopback responses."""
import argparse
import http.server
import os
from pathlib import Path
import subprocess
import tempfile
import threading
import time
from urllib.parse import urlsplit

NS = 'http://www.w3.org/1999/xhtml'
SVG = 'http://www.w3.org/2000/svg'


class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self, _format, *args):
        pass

    def do_GET(self):
        name = urlsplit(self.path).path
        if name in ['/redirect', '/foreign']:
            target = '/vector.html'
            if name == '/foreign':
                target = f'http://localhost:{self.server.server_port}/vector.html'
            self.send_response(302)
            self.send_header('Location', target)
            self.send_header('Content-Length', '0')
            self.end_headers()
            return
        routes = {
            '/vector.html': ('IMAGE/SVG+XML; charset=utf-8',
                             f'<?trace owned?><svg xmlns="{SVG}" xmlns:q="urn:actual" q:a="v"><g><![CDATA[A<B]]></g></svg>'),
            '/html.xml': ('text/html; charset=utf-8',
                          "<!doctype html><body><script>parent.notes.push('html-script');document.write('<p id=written>native</p>');document.addEventListener('DOMContentLoaded',function(){parent.notes.push('html-dcl');});window.addEventListener('load',function(){parent.notes.push('html-load');});</script></body>"),
            '/good.bin': ('text/xml; charset=utf-8',
                          f'<html xmlns="{NS}"><body><script>parent.notes.push("xml-script");parent.xmlOwner=(window.parent===parent &amp;&amp; top===parent &amp;&amp; document.defaultView===window);</script></body></html>'),
            '/wrong.xml': ('text/xml', f'<html xmlns="{NS}#"><script>parent.notes.push("wrong-script");</script></html>'),
            '/case.xml': ('text/xml', f'<html xmlns="{NS}"><Script>parent.notes.push("case-script");</Script></html>'),
            '/bad.xml': ('text/xml', f'<html xmlns="{NS}"><script>parent.notes.push("bad-script");</script><p></html>'),
            '/plain.html': ('text/plain', '<script>parent.notes.push("plain-script");</script>'),
            '/old.xml': ('application/xml', '<old/>'),
            '/new.xml': ('application/xml', '<new/>'),
            '/slow.xml': ('application/xml', '<slow/>'),
        }
        mime, body = routes.get(name, ('text/plain', 'missing'))
        if name in ['/old.xml', '/slow.xml']:
            time.sleep(0.15)
        payload = body.encode('utf-8')
        self.send_response(200)
        self.send_header('Content-Type', mime)
        self.send_header('Content-Length', str(len(payload)))
        self.send_header('Connection', 'close')
        self.end_headers()
        try:
            self.wfile.write(payload)
        except (BrokenPipeError, ConnectionResetError):
            pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--program', required=True)
    args = parser.parse_args()
    server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        with tempfile.TemporaryDirectory(prefix='ws074-child-') as directory:
            Path(directory, 'local.xml').write_text('<local/>')
            completed = subprocess.run([args.program,
                f'http://127.0.0.1:{server.server_port}/parent.html', directory],
                env=dict(os.environ, ASAN_OPTIONS='detect_stack_use_after_return=0'),
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=60)
            print(completed.stdout, end='')
            return completed.returncode
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=5)


if __name__ == '__main__':
    raise SystemExit(main())
