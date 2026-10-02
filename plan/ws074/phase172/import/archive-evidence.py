#!/usr/bin/env python3
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Preserve browser2 planning evidence as named source data, without canonical updates."""
import hashlib
import json
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[4]
folder = root / 'plan/ws074/phase172/import'
manifest_path = folder / 'manifest.json'
manifest = json.loads(manifest_path.read_text())
index = []
for entry in manifest['entries']:
    if entry['scope'] == 'excluded-board':
        entry['disposition'] = 'excluded-unrelated-current-board'
        continue
    if entry['scope'] not in ('plan-evidence', 'history-evidence', 'bug-evidence'):
        continue
    content = subprocess.check_output(['git', 'show', manifest['branch'] + ':' + entry['original']], cwd=root)
    suffix = '.source.txt' if entry['original'].endswith('.md') else ''
    path = folder / 'browser2-evidence' / (entry['original'] + suffix)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(content)
    entry['archived_path'] = str(path.relative_to(root))
    entry['archive_sha256'] = hashlib.sha256(content).hexdigest()
    entry['disposition'] = 'archived-source-pending-semantic-reconciliation'
    index.append(dict(original=entry['original'], archived=entry['archived_path'], sha256=entry['archive_sha256']))
manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
(folder / 'browser2-evidence/index.json').write_text(json.dumps(dict(source=manifest['branch'], entries=index), indent=2) + '\n')
print('archived source evidence:', len(index))
