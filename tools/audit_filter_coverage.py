#!/usr/bin/env python3
"""Verify pinned filter inventory, public exports and real consumer/unit coverage.

Optional --references also checks the freshly fetched Rust command sources under
build/references. The library/package has no Python runtime dependency.
"""
from pathlib import Path
import argparse
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--references', action='store_true')
args = parser.parse_args()
catalog = json.loads((ROOT / 'docs/filter-catalog.json').read_text())
entries = catalog['entries']
commands = [entry['command'] for entry in entries]
assert len(commands) == len(set(commands)) == 124, 'missing/duplicate command rows'
assert sum(e['kind'] == 'filter' for e in entries) == 120
assert sum(e['kind'] == 'edit' for e in entries) == 2
assert sum(e['kind'] == 'alias' for e in entries) == 2
assert len({e['api'] for e in entries}) == 122
assert len(catalog['lightcraft_looks']) == 22
assert len(catalog['lightcraft_presets']) == 41
assert len(catalog['resamplers']) == 4
cmake = (ROOT / 'CMakeLists.txt').read_text()
umbrella = (ROOT / 'src/iiGraphicAdjust/iiGraphicAdjust.h').read_text()
consumer = (ROOT / 'tests/consumer/filters.cpp').read_text()
unit = '\n'.join(p.read_text() for p in (ROOT / 'tests').glob('*.cpp'))
modules = ['BlurFilters', 'DistortionFilters', 'UtilityFilters', 'GalleryFilters',
           'PixelFilters', 'RenderFilters', 'LensFilter', 'CameraRawFilter',
           'MeshFilters', 'LightcraftFilters', 'RetouchFilters', 'DiagnosticFilters']
exports = {}
for module in modules:
    header = module + '.h'
    body = (ROOT / 'src/iiGraphicAdjust' / header).read_text()
    assert header in cmake and header in umbrella, f'not installed/included: {header}'
    assert module + '.cpp' in cmake, f'not built: {module}'
    for name in re.findall(r'IIGRAPHICADJUST_EXPORT\s+Result<.*?>\s+(\w+)\(', body, re.S):
        assert name not in exports, f'duplicate export: {name}'
        exports[name] = header
        assert re.search(r'\b' + name + r'\s*\(', consumer), f'not called by installed consumer: {name}'
        assert re.search(r'\b' + name + r'\b', unit), f'no unit coverage: {name}'
for entry in entries:
    assert exports.get(entry['api']) == entry['header'], f'wrong API/header: {entry}'
    assert f"`{entry['command']}`" in (ROOT / 'docs/Filters.md').read_text()
for entry in catalog['lightcraft_retouch']:
    assert exports.get(entry['api']) == 'RetouchFilters.h', f'missing retouch function: {entry}'
for entry in catalog['lightcraft_diagnostics']:
    assert exports.get(entry['api']) == 'DiagnosticFilters.h', f'missing diagnostic function: {entry}'
for name in catalog['lightcraft_looks'] + catalog['lightcraft_presets'] + catalog['resamplers']:
    assert exports.get(name) == 'LightcraftFilters.h', f'missing LightCraft function: {name}'

if args.references:
    base = ROOT / 'build/references'
    photocraft = base / 'photocraft'
    lightcraft = base / 'lightcraft'
    for repo, commit in [(photocraft, catalog['reference_commit']),
                         (lightcraft, catalog['lightcraft_commit'])]:
        observed = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=repo, text=True).strip()
        assert observed == commit, f're-inspect changed reference: {repo.name} {observed}'
    actual = set()
    for path in (photocraft / 'crates/engine/src').glob('*.rs'):
        actual.update(re.findall(r'"(filter\.[A-Za-z0-9_.]+)"', path.read_text()))
    # Host orchestration, the dynamic Gallery prefix, and an invalid-command test fixture.
    actual -= {'filter.convertForSmartFilters', 'filter.filterGallery',
               'filter.gallery.', 'filter.lastFilter', 'filter.nope'}
    artistic = (photocraft / 'crates/algo/src/artistic.rs').read_text()
    gallery = re.findall(r'^\s*\w+\s*=\s*"(\w+)",', artistic, re.M)
    assert len(gallery) == 47 and len(set(gallery)) == 47
    actual.update('filter.gallery.' + name for name in gallery)
    expected = {e['command'] for e in entries if e['kind'] == 'filter'}
    assert actual == expected, f'command inventory changed: missing={actual-expected}, extra={expected-actual}'
    for entry in catalog['lightcraft_retouch'] + catalog['lightcraft_diagnostics']:
        assert (lightcraft / entry['file']).is_file(), f'bad LightCraft source: {entry}'
    for entry in entries:
        path = photocraft / entry['file']
        assert path.is_file() and len(path.read_text().splitlines()) >= entry['line']
print(f'Coverage passed: 120 Filter + 2 Edit + 2 alias registrations, '
      f'22 looks + 41 presets + 4 resamplers, {len(exports)} new exported APIs; '
      f'installed consumer and unit coverage present.')
