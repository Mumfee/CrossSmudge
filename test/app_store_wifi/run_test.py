#!/usr/bin/env python3
"""Execute production AppStore UI/control flow with recording hardware boundaries.

The backend methods are replaced because they require ESP32/storage/HTTP. The
render, input, lifecycle entry, connection callback, and button implementation
come directly from production at test time, so behavior changes are exercised.
"""
import pathlib
import os
import re
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
HERE = pathlib.Path(__file__).resolve().parent


def no_includes(text):
    return re.sub(r'^\s*#(?:include|pragma)[^\n]*', '', text, flags=re.M)


def function_end(source, start):
    tokens = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*.*?\*/|[{}]', re.S)
    depth = 0
    for token in tokens.finditer(source, start):
        if token.group() == '{':
            depth += 1
        elif token.group() == '}':
            depth -= 1
            if depth == 0:
                return token.end()
    raise RuntimeError('Unbalanced method')


def extract_function(source, signature):
    start = source.index(signature)
    body = source.index('{', start)
    return source[start:function_end(source, body)]


def replace_body(source, name, replacement):
    match = re.search(r'^  (?:static )?(?:void|bool|int) ' + name + r'\([^\n]*\)[^{]*\{', source, re.M)
    if not match:
        raise RuntimeError('Backend seam missing: ' + name)
    start = match.end() - 1
    return source[:start] + '{' + replacement + '}' + source[function_end(source, start):]


source = no_includes((ROOT / 'src/activities/apps/AppStoreActivity.h').read_text())
for name in ['onExit', 'expand32To64', 'loadAppIcon', 'ensureAppIcon',
             'checkInstalledStatus', 'handleCatalogInput', 'handleDetailInput',
             'installApp', 'uninstallApp', 'renderCatalog', 'renderAppDetail',
             'renderDownloadProgress', 'renderError']:
    source = replace_body(source, name, '')
source = replace_body(source, 'getDetailActionButtonY', 'return 0;')
source = replace_body(source, 'isWifiConnected', 'return wifiConnected;')
source = replace_body(source, 'fetchCatalog', '++fetches; state_ = State::CATALOG_READY;')
buttons = no_includes((ROOT / 'src/components/TouchActionButtons.h').read_text())
buttons += no_includes((ROOT / 'src/components/TouchActionButtons.cpp').read_text())
header_source = (ROOT / 'src/components/TouchHeaderBackButton.cpp').read_text()
header = no_includes((ROOT / 'src/components/TouchHeaderBackButton.h').read_text())
header += '\nnamespace TouchHeaderBackButton {\n'
header += '\n'.join(re.findall(r'^constexpr int (?:actionSize|touchSize|titleGap) = .*;', header_source, re.M))
for signature in ['Layout layout(', 'int height(', 'bool wasTapped(const MappedInputManager& input, const Rect& header)']:
    header += '\n' + extract_function(header_source, signature)
header += '\nvoid draw(GfxRenderer&, const Rect&, const char*, bool, int, const char*, int) {}\n}\n'

with tempfile.TemporaryDirectory(prefix='app-store-wifi-') as tmp:
    cpp = pathlib.Path(tmp) / 'test.cpp'
    cpp.write_text((HERE / 'boundaries.h').read_text() + buttons + header + source + (HERE / 'behavior.cpp').read_text())
    binary = pathlib.Path(tmp) / 'test'
    subprocess.run(['c++', '-std=c++20', '-g', '-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer', str(cpp), '-o', str(binary)], check=True)
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
    subprocess.run([str(binary)], check=True, env=env)
