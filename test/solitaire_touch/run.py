#!/usr/bin/env python3
"""Build vendored Lua and exercise actual Solitaire sources in isolated copies.
Requires Python 3, a C compiler (CC, default cc), and libm. No firmware build.
--app also accepts an extracted release's solitaire directory.
"""
import argparse
import os
import re
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
CASES = ('stock', 'card', 'run', 'waste', 'empty', 'illegal', 'bounds',
         'cancel', 'reset', 'reset_edge', 'buttons', 'save', 'dispatch')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', type=Path, default=ROOT / 'apps/solitaire')
    parser.add_argument('--budget', type=int, default=2 * 1024 * 1024)
    parser.add_argument('--devices', nargs='+', default=['sticky', 'x4pro', 'x4', 'x3', 'simulator', 'missing'])
    args = parser.parse_args()
    total = 0
    failed = 0
    with tempfile.TemporaryDirectory(prefix='solitaire-touch-') as tmp:
        work = Path(tmp)
        binary = work / 'runner'
        lua = ROOT / 'lib/Lua/src'
        subprocess.run(shlex.split(os.environ.get('CC', 'cc')) + [
            '-O2', '-I' + str(lua), str(HERE / 'runner.c'),
            *map(str, sorted(lua.glob('*.c'))), '-lm', '-o', str(binary)], check=True)
        app = work / 'solitaire'
        shutil.copytree(args.app.resolve(), app)
        # Compile each scenario separately to limit test overhead.
        sections = re.split(r'^-- CASE (.+)\n', (HERE / 'cases.lua').read_text(), flags=re.M)
        scenarios = {}
        for names, body in zip(sections[1::2], sections[2::2]):
            for name in names.split():
                scenarios[name] = sections[0] + body + '\non_draw()\non_exit()\n'
        for device in args.devices:
            for width, height in ((480, 800), (800, 480)):
                for case in CASES:
                    if device != 'sticky' and case in ('bounds', 'reset_edge'):
                        continue  # Sticky-only geometry corrections; legacy checked separately.
                    total += 1
                    script = work / 'case.lua'
                    script.write_text(scenarios[case])
                    result = subprocess.run([str(binary), str(script),
                                             str(width), str(height), case, device, str(args.budget)], cwd=app)
                    failed += result.returncode != 0
    print(f'{total - failed}/{total} cases passed', flush=True)
    return bool(failed)


if __name__ == '__main__':
    raise SystemExit(main())
