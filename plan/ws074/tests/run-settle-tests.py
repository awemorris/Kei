#!/usr/bin/env python3
# zedBSD
# Copyright (C) 2026 Awe Morris
# SPDX-License-Identifier: Zlib
"""Check finite page task chains, virtual-time cutoffs and runaway rejection."""
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
    checked = 0
    with tempfile.TemporaryDirectory(prefix="ws074-settle-") as name:
        directory = Path(name)
        command, environment = ACID.command_base(
            args.program, ACID.fonts(), 400, 300, directory, "--run"
        )
        command = [word for word in command if not word.startswith("--settle-ms=")]

        def run(source, options=()):
            page = directory / "test.html"
            page.write_text("<!doctype html><script>" + source + "</script>",
                            encoding="utf-8")
            return subprocess.run(command + list(options) + [str(page)],
                                  env=environment, capture_output=True,
                                  text=True, timeout=30)

        chain = """
var count = 0;
window.addEventListener('message', function(event) {
  if (event.data !== 'next') return;
  count++;
  if (count === 300) console.log('CHAIN_DONE', count);
  else setTimeout(function() { window.postMessage('next', '*'); }, 10);
});
window.postMessage('next', '*');
"""
        result = run(chain)
        assert result.returncode == 0, result.stderr
        assert "CHAIN_DONE 300" in result.stdout, result.stdout
        assert "Uncaught" not in result.stdout + result.stderr
        checked += 1

        source = "setTimeout(function(){console.log('LATE_DONE');},6000);"
        result = run(source)
        assert result.returncode == 0 and "LATE_DONE" not in result.stdout
        checked += 1
        result = run(source, ["--settle-ms=7000"])
        assert result.returncode == 0 and "LATE_DONE" in result.stdout
        checked += 1

        result = run("setTimeout(function(){console.log('ZERO_DONE');},0);",
                     ["--settle-ms=0"])
        assert result.returncode == 0 and "ZERO_DONE" in result.stdout
        checked += 1

        result = run("function again(){setTimeout(again,0);} again();")
        assert result.returncode != 0, result.stdout
        assert "cannot run" in result.stderr, result.stderr
        checked += 1

        for invalid in ("", "-1", "+1", " 1", "1.5", "NaN", "Infinity",
                        "60001", "999999999999999999999999"):
            result = run("console.log('MUST_NOT_RUN');",
                         ["--settle-ms=" + invalid])
            assert result.returncode != 0
            assert "MUST_NOT_RUN" not in result.stdout
            checked += 1

    # Check that incomplete/duplicate diagnostics cannot certify all 100 tests.
    rows = [ACID.RESULT_PREFIX + '{"num_tests":100}']
    rows += [ACID.RESULT_PREFIX + '{"test":%d,"result":"pass"}' % index
             for index in range(100)]
    assert ACID.acid3_inventory(0, rows, [])['complete']
    assert not ACID.acid3_inventory(0, rows[:-1], [])['complete']
    assert not ACID.acid3_inventory(0, rows + rows[-1:], [])['complete']
    assert not ACID.acid3_inventory("timeout", rows, [])['complete']
    assert ACID.acid3_complete('| <html>\n|   class=""\n|   <head>\n')
    assert not ACID.acid3_complete(
        '| <html>\n|   class="reftest-wait other"\n|   <head>\n')
    assert not ACID.acid3_complete('')
    assert ACID.acid3_score(
        '| <span>\n|   id="score"\n|   class="cats"\n|   "32"\n') == "32"
    print("settle: %d runtime checks and 8 diagnostic checks passed" % checked)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
