#!/usr/bin/env python3
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Run a finite, validated browser2 regression command inventory on imported outputs."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import time

ROOT = Path(__file__).resolve().parents[4]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('variant', choices=('plain', 'asan'))
args = parser.parse_args()
variant = args.variant
source = ROOT / 'plan/ws074/phase172/import/browser2-evidence/plan/ws074/phase171/verification-q590.json'
inventory = json.loads(source.read_text())['regressions'][variant]
folder = ROOT / ('build/p10-regression-' + variant)
folder.mkdir(parents=True, exist_ok=True)
output = []
started = time.monotonic()


def save_report():
    (folder / 'report.json').write_text(json.dumps(dict(
        variant=variant, inventory_source=str(source.relative_to(ROOT)),
        results=output), indent=2) + '\n')


for index, item in enumerate(inventory):
    if time.monotonic() - started >= 900:
        raise RuntimeError('the complete regression inventory exhausted its 900-second bound')
    command = item['command']
    if command == ['selectors style golden']:
        output.append(dict(group=index, source_command=command, status='pending-golden-comparison'))
        continue
    command = [arg.replace('build/ws074-host/', 'build/p10-browser2/') for arg in command]
    first = command[0]
    if first == 'python3':
        allowed = ('run-frame-load-tests.py', 'run-resource-tests.py', 'run-dom-tests.py', 'run-js-tests.py')
        if command[1] not in ['plan/ws074/tests/' + name for name in allowed]:
            raise RuntimeError('unreviewed Python runner')
    elif not re.fullmatch(r'build/p10-browser2/' + variant + r'/(host-[a-z-]+|browser)', first):
        raise RuntimeError('unreviewed executable')
    if any(any(token in arg for token in (';', '|', '`', '$(', '.internal')) for arg in command):
        raise RuntimeError('unreviewed argument')
    env = dict(os.environ, ASAN_OPTIONS='detect_stack_use_after_return=0',
               XDG_DATA_HOME=str(folder / 'data'))
    try:
        result = subprocess.run(command, cwd=ROOT, env=env, capture_output=True,
                                text=True, errors='replace', timeout=120)
        log = result.stdout + result.stderr
        failed = result.returncode != 0 or bool(re.search(r'\b(?:FAIL|Uncaught|AddressSanitizer|runtime error:)\b', log))
        status = 'failed' if failed else 'passed'
        code = result.returncode
    except subprocess.TimeoutExpired as error:
        log, status, code = str(error), 'timeout', None
    except OSError as error:
        log, status, code = str(error), 'runner-error', None
    path = folder / ('group-%03d.log' % index)
    path.write_text(log)
    output.append(dict(group=index, command=command, status=status, exit=code,
                       log=str(path.relative_to(ROOT)), log_sha256=hashlib.sha256(log.encode()).hexdigest(),
                       last=log.splitlines()[-3:]))
    save_report()
    print(index, status, ' '.join(command), flush=True)
save_report()
print('passed',sum(e['status']=='passed' for e in output),'/',len(output))
raise SystemExit(1 if any(e['status'] in ('failed','timeout','runner-error') for e in output) else 0)
