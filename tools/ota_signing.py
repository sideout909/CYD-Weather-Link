"""Byte-exact signed OTA envelope; OpenSSL does all cryptographic operations."""
import hashlib
import re
import subprocess
from pathlib import Path

ALGORITHM = 'ecdsa-p256-sha256'
FIELDS = ('signing_key_id', 'board', 'partition_layout', 'flash_bytes', 'version',
          'channel', 'size', 'sha256', 'firmware_url')

def public_der(private_key):
    return subprocess.check_output(['openssl', 'pkey', '-in', str(private_key), '-pubout', '-outform', 'DER'], stderr=subprocess.PIPE)

def key_id(der):
    return hashlib.sha256(der).hexdigest()[:32]

def payload(manifest):
    if manifest.get('schema') != 1 or manifest.get('signature_schema') != 1 or manifest.get('signature_algorithm') != ALGORITHM:
        raise ValueError('Unsupported signature schema/algorithm')
    values = [str(manifest[name]) for name in FIELDS]
    if any(not value or not value.isascii() or '\n' in value or '\r' in value for value in values):
        raise ValueError('Invalid signed envelope fields')
    return ('WX-Link OTA signature v1\n' + ALGORITHM + '\n' + '\n'.join(values) + '\n').encode('ascii')

def sign(manifest, private_key, trust_header):
    der = public_der(private_key)
    # Refuse mismatched/unreviewed keys instead of publishing an uninstallable image.
    configured = re.findall(r'key_id="([0-9a-f]{32})"', Path(trust_header).read_text())
    if key_id(der) not in configured:
        raise ValueError('Signing key does not match the firmware trust header')
    header = Path(trust_header).read_text()
    public = re.search(r'-----BEGIN PUBLIC KEY-----.*?-----END PUBLIC KEY-----', header, re.S)
    if not public:
        raise ValueError('Missing public trust key')
    trusted = subprocess.check_output(['openssl', 'pkey', '-pubin', '-outform', 'DER'], input=public[0].encode(), stderr=subprocess.PIPE)
    if der != trusted:
        raise ValueError('Private key does not match embedded public trust key')
    details = subprocess.check_output(['openssl', 'pkey', '-pubin', '-inform', 'DER', '-text', '-noout'], input=der, stderr=subprocess.PIPE)
    if b'prime256v1' not in details:
        raise ValueError('Signing key must use ECDSA P-256')
    manifest.update(schema=1, signature_schema=1, signing_key_id=key_id(der), signature_algorithm=ALGORITHM)
    signature = subprocess.run(['openssl', 'dgst', '-sha256', '-sign', str(private_key)],
                               input=payload(manifest), capture_output=True, check=True).stdout
    manifest['signature'] = signature.hex()
    return manifest
