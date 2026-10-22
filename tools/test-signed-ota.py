#!/usr/bin/env python3
"""Test actual firmware mbedTLS verifier with ephemeral keys, never publisher secrets.

Provide host mbedTLS 2.28.7 source/build trees (firmware's pinned version), compiled
with cmake -DENABLE_TESTING=OFF -DENABLE_PROGRAMS=OFF; target mbedcrypto.
"""
import argparse
import copy
import json
from pathlib import Path
import subprocess
import tempfile
from ota_signing import sign, payload

ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--mbedtls-source',type=Path,required=True)
p.add_argument('--mbedtls-build',type=Path,required=True)
a=p.parse_args()

def run(args,**kwargs):
    result=subprocess.run(list(map(str,args)),capture_output=True,**kwargs)
    if result.returncode:
        raise RuntimeError(result.stderr.decode(errors='replace'))
    return result

with tempfile.TemporaryDirectory(prefix='wxlink-signed-ota-') as name:
    tmp=Path(name);key=tmp/'ephemeral.key';trust=tmp/'trust.h'
    run(['python3',ROOT/'tools/create-ota-signing-key.py','--private-key',key,'--header',trust])
    exe=tmp/'verify'
    run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
         '-DWX_LINK_OTA_TRUST_HEADER="'+str(trust)+'"','-I'+str(ROOT/'firmware/include'),
         '-I'+str(ROOT/'firmware/.pio/libdeps/panellan_sc05_x/ArduinoJson/src'),
         '-I'+str(a.mbedtls_source/'include'),ROOT/'firmware/tests/signed_ota_test.cpp',
         a.mbedtls_build/'library/libmbedcrypto.a','-o',exe])
    manifest=dict(schema=1,signature_schema=1,board='panellan_sc05_x',partition_layout='default_8MB-v1',
                  flash_bytes=8388608,version='0.9.0-rc.1',channel='dev',size=1800000,
                  sha256='ab'*32,firmware_url='https://github.com/sideout909/WX-Link/releases/download/v0.9.0-rc.1/weather-link-panellan_sc05_x.bin')
    sign(manifest,key,trust)
    def verify(value,expected=True,repo='sideout909/WX-Link'):
        result=subprocess.run([str(exe),repo],input=json.dumps(value).encode(),capture_output=True)
        assert (result.returncode==0)==expected, (value,result.stderr)
    verify(manifest)
    verify(dict(reversed(list(manifest.items()))))
    mutations={'schema':2,'signature_schema':2,'signature_algorithm':'none','signing_key_id':'0'*32,
               'board':'other_board','partition_layout':'other-layout','flash_bytes':4194304,
               'version':'0.9.0','channel':'stable','size':1800001,'sha256':'cd'*32,
               'firmware_url':manifest['firmware_url'].replace('WX-Link/','attacker/'),
               'signature':'00'*72}
    for field,value in mutations.items():
        bad=copy.deepcopy(manifest);bad[field]=value;verify(bad,False)
    for signature in ('',manifest['signature'][:-2],manifest['signature']+'00','z'*144,'a'*161):
        bad=dict(manifest,signature=signature);verify(bad,False)
    for field in ('signature','signing_key_id','channel','size'):
        bad=copy.deepcopy(manifest);del bad[field];verify(bad,False)
    verify(manifest,False,repo='owner/fork')
    # A valid envelope from another private key must not gain publisher trust.
    other=tmp/'other.key';othertrust=tmp/'other.h'
    run(['python3',ROOT/'tools/create-ota-signing-key.py','--private-key',other,'--header',othertrust])
    bad=dict(manifest);sign(bad,other,othertrust);verify(bad,False)
    try:sign(dict(manifest),other,trust)
    except ValueError:pass
    else:raise AssertionError('Packager accepted an untrusted publisher key')
    # Independently check canonical C++ construction by verifying Python/OpenSSL
    # signatures through the actual firmware verifier above, then stable envelopes.
    stable=dict(manifest,version='0.9.0',channel='stable',firmware_url=manifest['firmware_url'].replace('v0.9.0-rc.1/','v0.9.0/'))
    sign(stable,key,trust);verify(stable)
    assert payload(manifest).endswith(b'\n')
    print('PASS: actual mbedTLS firmware verifier accepts valid Stable/Dev signatures; rejects tampered metadata, wrong keys/repository, missing/invalid signatures and schema/algorithm changes.')
