#!/usr/bin/env python3
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Classify the fixed browser2 diff against the WS107 relocation inventory."""
import hashlib
import json
import pathlib
import subprocess
import tempfile
from collections import Counter

ROOT = pathlib.Path(__file__).resolve().parents[4]
COMMON = '493b6eea90c45b3c1f393c0c62a0f7882b43621c'
BRANCH = 'e53ef03b80113aec959deb67f828cba21d68d4be'
CURRENT = '41aac4fc76d0038f9024b0b966d8ae93eb4145fa'
OUT = ROOT / 'plan/ws074/phase172/import'
OUT.mkdir(parents=True, exist_ok=True)
(ROOT / 'plan/ws074/temp/p172').mkdir(parents=True, exist_ok=True)


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)


def blob(revision, path):
    listing = git('ls-tree', '-r', '--name-only', revision, '--', path)
    if not listing:
        return None
    return git('show', revision + ':' + path)


def normalize(content, old, new):
    if old.startswith('userland/desktop/browser/') and new.startswith('userland/desktop/libbrowser/'):
        return content.replace(b'userland/desktop/browser/', b'userland/desktop/libbrowser/')
    if old == 'userland/desktop/libbrowser/Makefile':
        content = content.replace(b'$(KEILAND_BROWSER_DIR)', b'$(LIBBROWSER_DIR)')
        content = content.replace(b'userland/desktop/browser', b'userland/desktop/libbrowser')
    return content


mapping = {entry['before']: entry['after'] for entry in json.loads(
    (ROOT / 'plan/history/ws107/inventory.json').read_text())['files']}
rows = []
for line in git('diff', '--name-status', COMMON, BRANCH).decode().splitlines():
    status, old = line.split('\t')
    new = mapping.get(old, old)
    if old.startswith('userland/desktop/browser/'):
        owner = 'shell' if old == 'userland/desktop/browser/main.c' or '/shell/' in old or '/data/' in old else 'engine'
        if old not in mapping and owner == 'engine':
            new = old.replace('userland/desktop/browser/', 'userland/desktop/libbrowser/', 1)
        scope = 'source'
    elif old == 'userland/desktop/libbrowser/Makefile':
        owner, scope = 'engine', 'build'
    elif old.startswith('plan/ws074/tests/'):
        owner, scope = 'WS074', 'test'
    elif old.startswith('plan/ws074/'):
        owner, scope = 'main-reconciliation', 'plan-evidence'
    elif old.startswith('plan/bugs/'):
        owner, scope = 'main-reconciliation', 'bug-evidence'
    elif old.startswith('plan/history/'):
        owner, scope = 'main-reconciliation', 'history-evidence'
    else:
        owner, scope = 'main', 'excluded-board'
    versions = dict(common=blob(COMMON, old), branch=blob(BRANCH, old), current=blob(CURRENT, new))
    if versions['branch'] is None and status != 'D':
        raise RuntimeError('branch path missing: ' + old)
    ancestor, branch, current = (versions[key] or b'' for key in ('common', 'branch', 'current'))
    classification = 'not-reviewed'
    if scope in ('source', 'build', 'test'):
        ancestor, branch = normalize(ancestor, old, new), normalize(branch, old, new)
        if branch == current:
            classification = 'redundant'
        elif versions['common'] is None and versions['current'] is None:
            classification = 'new'
        elif ancestor == current:
            classification = 'clean'
        else:
            with tempfile.TemporaryDirectory(dir=ROOT / 'plan/ws074/temp/p172') as directory:
                paths = [pathlib.Path(directory) / name for name in ('current', 'ancestor', 'branch')]
                for path, content in zip(paths, (current, ancestor, branch)):
                    path.write_bytes(content)
                merged = subprocess.run(['git', 'merge-file', '-p', *map(str, paths)],
                                        stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            classification = 'clean-three-way' if merged.returncode == 0 else 'conflict'
            if scope in ('source', 'build'):
                target = ROOT / 'plan/ws074/temp/p172/staged' / new
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(merged.stdout)
    row = dict(branch_status=status, original=old, target=new, owner=owner, scope=scope,
               classification=classification, disposition='unresolved')
    row['present'] = {key: value is not None for key, value in versions.items()}
    row['raw_sha256'] = {key: hashlib.sha256(value).hexdigest() if value is not None else None for key, value in versions.items()}
    for label, content in [('common', ancestor), ('branch', branch), ('current', current)]:
        row[label + '_sha256'] = hashlib.sha256(content).hexdigest()
    rows.append(row)
output = dict(common=COMMON, branch=BRANCH, current=CURRENT,
              hash_contract='comparison hashes normalize engine path/Makefile variables; raw_sha256 hashes original bytes; present distinguishes absent from empty',
              entries=rows)
(OUT / 'manifest.json').write_text(json.dumps(output, indent=2) + '\n')
print('scope:', dict(Counter(row['scope'] for row in rows)))
print('source/build:', dict(Counter(row['classification'] for row in rows if row['scope'] in ('source', 'build'))))
print('tests:', dict(Counter(row['classification'] for row in rows if row['scope'] == 'test')))
for row in rows:
    if row['classification'] == 'conflict':
        print('conflict:', row['target'])
