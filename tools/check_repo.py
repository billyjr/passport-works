#!/usr/bin/env python3
"""Check sources, upstream provenance, documentation and the upload boundary."""
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parent.parent
SECRETS = {
    'private key': rb'-----BEGIN (?:RSA |EC |OPENSSH |DSA )?PRIVATE KEY-----',
    'GitHub token': rb'(?:gh[pousr]_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{40,})',
    'AWS key': rb'(?:AKIA|ASIA)[A-Z0-9]{16}',
    'API key': rb'\bsk-(?:proj-|svcacct-)?[A-Za-z0-9_-]{32,}',
    'credential URL': rb'https?://[^\s/:<>]+:[^\s/@<>]+@',
}
def main():
    errors = []
    raw = subprocess.check_output(['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard'], cwd=ROOT)
    names = sorted(set(n.decode() for n in raw.split(b'\0') if n))
    for name in names:
        path = ROOT / name
        if path.is_symlink():
            errors.append(f'{name}: symlink is not a standalone source')
            continue
        if name.startswith(('assets/private/', 'build/', 'managed_components/')) or path.name.startswith('.env') or path.suffix in ('.pem', '.key', '.p12', '.pfx', '.log', '.bin', '.elf', '.map'):
            errors.append(f'{name}: private or generated file is eligible for upload')
        data = path.read_bytes()
        for label, pattern in SECRETS.items():
            if re.search(pattern, data):
                errors.append(f'{name}: possible {label} (value withheld)')
        if re.search(rb'(?m)^(<<<<<<< |=======\s*$|>>>>>>> )', data):
            errors.append(f'{name}: merge conflict marker')
        if path.suffix != '.md':
            continue
        text = data.decode('utf-8')
        counterpart = path.with_name(path.name.replace('.zh_CN.md', '.md')) if path.name.endswith('.zh_CN.md') else path.with_name(path.stem + '.zh_CN.md')
        if not counterpart.is_file() or counterpart.name not in text.splitlines()[0]:
            errors.append(f'{name}: missing paired document or reciprocal link')
        if not path.name.endswith('.zh_CN.md') and re.search(r'[\u3400-\u4dbf\u4e00-\u9fff]', '\n'.join(text.splitlines()[1:])):
            errors.append(f'{name}: English default document contains Chinese prose')
        for target in re.findall(r'!?\[[^\]]*\]\(([^)]+)\)', text):
            if target.startswith(('https://', 'http://', '#', 'mailto:')):
                continue
            target = unquote(target.split('#', 1)[0])
            resolved = ROOT / target.lstrip('/') if target.startswith('/') else path.parent / target
            if not resolved.exists():
                errors.append(f'{name}: broken local link {target}')
    manifest = json.loads((ROOT / 'docs/upstream.json').read_text())
    for name, expected in manifest['files'].items():
        p = ROOT / name
        if not p.is_file() or hashlib.sha256(p.read_bytes()).hexdigest() != expected:
            errors.append(f'{name}: differs from recorded upstream snapshot')
    for workflow in (ROOT / '.github/workflows').glob('*.yml'):
        for action in re.findall(r'uses:\s*(\S+)', workflow.read_text()):
            if not re.fullmatch(r'[^\s]+@[0-9a-f]{40}', action):
                errors.append(f'{workflow.name}: action is not pinned')
    if errors:
        print('\n'.join(errors), file=sys.stderr)
        return 1
    print(f'Repository/security checks: PASS ({len(names)} publishable files)')
    return 0
if __name__ == '__main__':
    raise SystemExit(main())
