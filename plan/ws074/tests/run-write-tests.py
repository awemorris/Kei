#!/usr/bin/env python3
# zedBSD
# Copyright (C) 2026 Awe Morris
# SPDX-License-Identifier: Zlib
"""Check parser-write lifetime, GC retention and finite recursion bounds."""
import argparse
import importlib.util
from pathlib import Path
import subprocess
import tempfile

SPEC = importlib.util.spec_from_file_location(
    "acid_runner", Path(__file__).with_name("run-acid-tests.py")
)
ACID = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ACID)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--program", type=Path,
                        default=ACID.ROOT / "build/ws074-host/plain/browser")
    args = parser.parse_args()
    fixtures = [
        ("late", """<!doctype html><body><p id=keep>kept</p><script>
setTimeout(function() {
    try { document.write('<p>replacement</p>'); }
    catch (error) { console.log('LATE', error.name); }
    console.log('KEPT', document.getElementById('keep').textContent);
}, 0);
</script>""", ["LATE NotSupportedError", "KEPT kept"]),
        ("bound", r"""<!doctype html><body><script>
var written = '<script>again();<\/script>';
var count = 0;
function again() {
    count++;
    try { document.write(written); }
    catch (error) { console.log('BOUND', error.name); }
}
again();
console.log('FINITE', count > 1 && count < 1000);
</script><p id=after>after</p><script>
console.log('RESUMED', document.getElementById('after').textContent);
</script>""", ["BOUND RangeError", "FINITE true", "RESUMED after"]),
        ("gc", """<!doctype html><body><div id=written><script>
for (var round = 0; round < 80; round++) {
    document.write('<span id="chunk' + round + '">' + round + '</span>');
    for (var index = 0; index < 300; index++) {
        var garbage = document.createElement('div');
        garbage.textContent = 'garbage ' + round + '-' + index;
        garbage.extra = {round: round, list: [index, index + 1]};
    }
}
</script></div><p id=tail>end</p><script>
console.log('CHUNKS', document.getElementById('written').children.length);
console.log('RETAINED', document.getElementById('chunk0').textContent,
            document.getElementById('chunk79').textContent);
console.log('TAIL', document.getElementById('tail').textContent);
</script>""", ["CHUNKS 81", "RETAINED 0 79", "TAIL end"]),
    ]
    with tempfile.TemporaryDirectory(prefix="ws074-write-") as name:
        directory = Path(name)
        command, environment = ACID.command_base(
            args.program, ACID.fonts(), 400, 300, directory, "--run"
        )
        for label, source, expected in fixtures:
            page = directory / (label + '.html')
            page.write_text(source, encoding='utf-8')
            result = subprocess.run(command + [str(page)], env=environment,
                                    capture_output=True, text=True, timeout=30)
            assert result.returncode == 0, (label, result.stderr)
            assert result.stdout.splitlines() == expected, (label, result.stdout)
            assert not result.stderr, (label, result.stderr)
            print('write ' + label + ': PASS')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
