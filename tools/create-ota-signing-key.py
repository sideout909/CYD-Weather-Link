#!/usr/bin/env python3
"""Create a local publisher key outside the repository and export only public trust."""
import argparse
import os
from pathlib import Path
import subprocess
from ota_signing import public_der, key_id

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--private-key', type=Path, required=True)
p.add_argument('--header', type=Path, default=ROOT/'firmware/include/ota_trust.h')
a = p.parse_args()
private = a.private_key.expanduser().resolve()
if private.is_relative_to(ROOT):
    p.error('Private signing key must be stored outside this repository')
if private.exists() or a.header.exists():
    p.error('Refusing to overwrite an existing private key or trust header')
private.parent.mkdir(parents=True, mode=0o700, exist_ok=True)
fd = os.open(private, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
try:
    with os.fdopen(fd, 'wb') as output:
        subprocess.run(['openssl', 'genpkey', '-algorithm', 'EC', '-pkeyopt', 'ec_paramgen_curve:prime256v1'], stdout=output, stderr=subprocess.PIPE, check=True)
    der = public_der(private)
    pem = subprocess.check_output(['openssl', 'pkey', '-in', str(private), '-pubout'], stderr=subprocess.PIPE).decode()
    a.header.write_text('#pragma once\n// Public publisher key only. Private key is never stored in firmware/source.\nnamespace ota_trust {\ninline constexpr const char* key_id="'+key_id(der)+'";\ninline constexpr const char public_key[]=R"KEY('+pem+')KEY";\n}\n')
except Exception:
    private.unlink(missing_ok=True)
    raise
print('Created private key outside the repository; public trust header exported. Back up the private key offline before publication.')
