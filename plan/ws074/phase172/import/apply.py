#!/usr/bin/env python3
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Apply only source/build/test entries whose fixed import classification is clean."""
import hashlib
import json
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[4]
MANIFEST = ROOT / 'plan/ws074/phase172/import/manifest.json'
manifest = json.loads(MANIFEST.read_text())
mapping = {entry['before']: entry['after'] for entry in json.loads(
    (ROOT / 'plan/history/ws107/inventory.json').read_text())['files']}
for entry in manifest['entries']:
    if entry['scope'] not in ('source', 'build', 'test'):
        continue
    if entry['classification'] not in ('clean', 'new', 'clean-three-way', 'redundant'):
        raise RuntimeError('unresolved classification: ' + entry['target'])
    target = ROOT / entry['target']
    current = target.read_bytes() if target.exists() else b''
    if hashlib.sha256(current).hexdigest() != entry['current_sha256']:
        raise RuntimeError('changed target: ' + entry['target'])
    content = subprocess.check_output(['git', 'show', manifest['branch'] + ':' + entry['original']], cwd=ROOT)
    if entry['classification'] == 'clean-three-way':
        content = (ROOT / 'plan/ws074/temp/p172/staged' / entry['target']).read_bytes()
    elif entry['scope'] == 'source':
        if entry['owner'] == 'engine':
            content = content.replace(b'userland/desktop/browser/', b'userland/desktop/libbrowser/')
    elif entry['scope'] == 'test':
        for old, new in mapping.items():
            if old != new:
                content = content.replace(old.encode(), new.encode())
        # Tests added on browser2 may refer to new engine files outside the old inventory.
        content = content.replace(b'userland/desktop/browser/xml/', b'userland/desktop/libbrowser/xml/')
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(content)
    mode = subprocess.check_output(['git', 'ls-tree', manifest['branch'], '--', entry['original']], cwd=ROOT).decode().split()[0]
    target.chmod(0o755 if mode == '100755' else 0o644)
    entry['disposition'] = 'applied-unverified'
    entry['applied_sha256'] = hashlib.sha256(content).hexdigest()
MANIFEST.write_text(json.dumps(manifest, indent=2) + '\n')
print('applied source/build/test:', sum(e['disposition'] == 'applied-unverified' for e in manifest['entries']))
